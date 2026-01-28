// 000101001
// 001000000

#include "scene.hpp"
#include "scene_preset.hpp"

#include <utility>
#include <iostream>
#include <cstring>
#include <cstdio>
#include <cassert>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>


constexpr size_t OBJECT_HEADER_SIZE = sizeof(unsigned int) + sizeof(int);
constexpr size_t LIGHT_HEADER_SIZE = sizeof(float);

void Scene::init() {
    assert(ctx && ctx->engine);
    VkSmol &engine = *ctx->engine;
    sphereBuffers.init(engine, sizeof(GpuSphere));
    planeBuffers.init(engine, sizeof(GpuPlane));
    boxBuffers.init(engine, sizeof(GpuBox));
    vertexBuffers.init(engine, sizeof(Vertex));
    indexBuffers.init(engine, sizeof(unsigned int));
    bvhBuffers.init(engine, sizeof(GpuBvhNode));
    meshBuffers.init(engine, sizeof(GpuMesh));

    materialBuffers.init(engine, sizeof(Material));
    objectBuffers.init(engine, sizeof(ObjectHandle), OBJECT_HEADER_SIZE);
    lightBuffers.init(engine, sizeof(GpuLight), LIGHT_HEADER_SIZE);

    auto& uiReg = ecs::ComponentUiRegistry::get();
    uiReg.setMaterials(&materials);
    uiReg.setMeshAssets(&meshAssets);
    ecs::ComponentUiRegistry::init();

    pushMaterial(DEFAULT_MATERIAL);
    meshAssets.push_back(DEFAULT_MESH_ASSET);
}

void Scene::destroy() {
    assert(ctx && ctx->engine);
    VkSmol &engine = *ctx->engine;
    sphereBuffers.destroy(engine);
    planeBuffers.destroy(engine);
    boxBuffers.destroy(engine);
    vertexBuffers.destroy(engine);
    indexBuffers.destroy(engine);
    bvhBuffers.destroy(engine);
    meshBuffers.destroy(engine);

    materialBuffers.destroy(engine);
    objectBuffers.destroy(engine);
    lightBuffers.destroy(engine);
}

void Scene::clear() {
    assert(ctx && ctx->engine);
    VkSmol &engine = *ctx->engine;
    engine.waitIdle();
    
    sphereBuffers.clear(engine);
    planeBuffers.clear(engine);
    boxBuffers.clear(engine);
    vertexBuffers.clear(engine);
    indexBuffers.clear(engine);
    bvhBuffers.clear(engine);
    meshBuffers.clear(engine);

    materialBuffers.clear(engine);
    objectBuffers.clear(engine);
    lightBuffers.clear(engine);

    objects.clear();
    materials.clear();
    meshAssets.clear();
    selectedEntity = -1;
    selectedMeshAsset = -1;
    bufferUpdated = true;

    pushMaterial(DEFAULT_MATERIAL);
    meshAssets.push_back(DEFAULT_MESH_ASSET);
}

LightMode Scene::loadPreset(ScenePreset preset) {
    LightMode mode = LightMode::Day;
    switch (preset) {
        case ScenePreset::Empty:
            initEmpty(*this, mode);
            break;
        case ScenePreset::Mesh:
            initMesh(*this, mode);
            break;
        case ScenePreset::Sponza:
            initSponza(*this, mode);
            break;
        case ScenePreset::CornellBox:
            initCornellBox(*this, mode);
            break;
        case ScenePreset::RandomSpheres:
            initRandomSpheres(*this, mode);
            break;
    }
    return mode;
}

CameraHandle* Scene::getFirstCameraHandle() const {
    for (Object* object : objects) {
        if (object->getType() == ObjectType::Camera) {
            return static_cast<CameraHandle*>(object);
        }
    }
    return nullptr;
}

MaterialHandle Scene::pushMaterial(const Material &mat) {
    assert(ctx && ctx->engine);
    VkSmol &engine = *ctx->engine;
    bufferUpdated |= materialBuffers.addElement(engine);
    const MaterialHandle materialHandle = static_cast<int>(materials.size());
    materials.push_back(mat);
    return materialHandle;
}

void Scene::pushSphere(std::string name, glm::vec3 center, float radius, MaterialHandle materialHandle) {
    assert(ctx && ctx->engine);
    VkSmol &engine = *ctx->engine;
    bufferUpdated |= sphereBuffers.addElement(engine);
    bufferUpdated |= objectBuffers.addElement(engine);

    ecs::Entity e = registry.createEntity();
    
    registry.add<ecs::Selectable>(e, ecs::Selectable{});

    ecs::Name nameComponent;
    nameComponent.setValue(std::move(name));
    registry.add<ecs::Name>(e, nameComponent);

    ecs::Sphere sphereComponent;
    sphereComponent.setRadius(radius);
    registry.add<ecs::Sphere>(e, sphereComponent);

    ecs::MaterialRef materialRef;
    materialRef.setHandle(materialHandle);
    registry.add<ecs::MaterialRef>(e, materialRef);
    
    ecs::Transform transformComponent;
    transformComponent.setPosition(center);
    transformComponent.setRotationToggle(false);
    transformComponent.setScaleToggle(false);
    registry.add<ecs::Transform>(e, transformComponent);

    entities.push_back(e);
}

void Scene::pushPlane(std::string name, glm::vec3 point, glm::vec3 normal, MaterialHandle materialHandle) {
    assert(ctx && ctx->engine);
    VkSmol &engine = *ctx->engine;
    bufferUpdated |= planeBuffers.addElement(engine);
    bufferUpdated |= objectBuffers.addElement(engine);

    ecs::Entity e = registry.createEntity();
    
    registry.add<ecs::Selectable>(e, ecs::Selectable{});

    ecs::Name nameComponent;
    nameComponent.setValue(std::move(name));
    registry.add<ecs::Name>(e, nameComponent);

    registry.add<ecs::Plane>(e, ecs::Plane{});

    ecs::MaterialRef materialRef;
    materialRef.setHandle(materialHandle);
    registry.add<ecs::MaterialRef>(e, materialRef);
    
    ecs::Transform transformComponent;
    transformComponent.setPosition(point);
    transformComponent.setRotation(glm::rotation(glm::vec3(0.0f, 1.0f, 0.0f), normal));
    transformComponent.setScaleToggle(false);
    registry.add<ecs::Transform>(e, transformComponent);

    entities.push_back(e);
}

void Scene::pushBox(std::string name, glm::vec3 cornerMin, glm::vec3 cornerMax, MaterialHandle materialHandle) {
    assert(ctx && ctx->engine);
    VkSmol &engine = *ctx->engine;
    glm::vec3 center = (cornerMin + cornerMax) * 0.5f;
    glm::vec3 halfExtents = (cornerMax - cornerMin) * 0.5f;
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), center);
    transform = glm::scale(transform, halfExtents);
    
    bufferUpdated |= boxBuffers.addElement(engine);
    bufferUpdated |= objectBuffers.addElement(engine);

    ecs::Entity e = registry.createEntity();
    
    registry.add<ecs::Selectable>(e, ecs::Selectable{});

    ecs::Name nameComponent;
    nameComponent.setValue(std::move(name));
    registry.add<ecs::Name>(e, nameComponent);

    registry.add<ecs::Box>(e, ecs::Box{});

    ecs::MaterialRef materialRef;
    materialRef.setHandle(materialHandle);
    registry.add<ecs::MaterialRef>(e, materialRef);
    
    ecs::Transform transformComponent;
    transformComponent.setPosition(center);
    transformComponent.setScale(halfExtents);
    transformComponent.updateLocal();
    registry.add<ecs::Transform>(e, transformComponent);

    entities.push_back(e);
}

void Scene::pushMesh(std::string name, const std::string &path, const glm::mat4 &transform, MaterialHandle materialHandle) {
    MeshHandle handle = static_cast<MeshHandle>(meshAssets.size());
    meshAssets.emplace_back(MeshAsset(name));
    if (!meshAssets.back().loadFromObj(*ctx, path)) {
        meshAssets.pop_back();
        return;
    }

    pushMesh(name, handle, transform, materialHandle);
}

void Scene::pushMesh(std::string name, MeshHandle meshHandle, const glm::mat4 &transform, MaterialHandle materialHandle) {
    if (!ctx || !ctx->engine) return;

    bufferUpdated |= meshBuffers.addElement(*ctx->engine);
    bufferUpdated |= objectBuffers.addElement(*ctx->engine);

    ecs::Entity e = registry.createEntity();
    registry.add<ecs::Selectable>(e, ecs::Selectable{});

    ecs::Name nameComponent;
    nameComponent.setValue(std::move(name));
    registry.add<ecs::Name>(e, nameComponent);

    ecs::MeshRef meshRef;
    meshRef.setHandle(meshHandle);
    registry.add<ecs::MeshRef>(e, meshRef);

    ecs::MaterialRef materialRef;
    materialRef.setHandle(materialHandle);
    registry.add<ecs::MaterialRef>(e, materialRef);

    glm::vec3 translation, rotationEuler, scale;
    ImGuizmo::DecomposeMatrixToComponents(
        glm::value_ptr(transform),
        glm::value_ptr(translation),
        glm::value_ptr(rotationEuler),
        glm::value_ptr(scale));

    ecs::Transform transformComponent;
    transformComponent.setPosition(translation);
    transformComponent.setRotation(glm::quat(glm::radians(rotationEuler)));
    transformComponent.setScale(scale);
    transformComponent.updateLocal();
    registry.add<ecs::Transform>(e, transformComponent);

    entities.push_back(e);
}

void Scene::pushCameraHandle(std::string name, glm::vec3 position, glm::vec3 direction, float fov) {
    CameraHandle *cameraHandle = new CameraHandle(name, position, direction, fov);
    if (previewCameraCallback)
        cameraHandle->setPreviewCallback(previewCameraCallback);
    objects.push_back(cameraHandle);
    updated = true;
}

// TODO refactor this
inline void addLight(const Material &mat, const float &area, const int &objectId, int &lightCount, std::vector<GpuLight> &lights, float &totalLightArea) {
    if (mat.type == MaterialType::Emissive) {
        lightCount++;
        lights.push_back(GpuLight{
            .objectId = objectId,
            .area = area,
            .pdfA = 1.0f/area,
        });
        totalLightArea += area;
    }
};

// TODO: Only refill them after an update (not every frame)
void Scene::fillBuffers() {
    assert(ctx && ctx->engine);
    VkSmol &engine = *ctx->engine;
    const auto& sphereStorage = registry.storage<ecs::Sphere>();
    const auto& planeStorage = registry.storage<ecs::Plane>();
    const auto& boxStorage = registry.storage<ecs::Box>();
    const auto& meshStorage = registry.storage<ecs::MeshRef>();
    const auto& transformStorage = registry.storage<ecs::Transform>();
    const auto& materialStorage = registry.storage<ecs::MaterialRef>();

    size_t totalVertices = 0;
    size_t totalIndices = 0;
    size_t totalBvhNodes = 0;

    for (const ecs::Entity& e : entities) {
        if (!meshStorage.has(e)) continue;
        const ecs::MeshRef& ref = meshStorage.get(e);
        if (ref.handle < 0 || static_cast<size_t>(ref.handle) >= meshAssets.size()) continue;
        
        const MeshAsset& asset = meshAssets[ref.handle];
        totalVertices += asset.getVertices().size();
        totalIndices += asset.getIndices().size();
        totalBvhNodes += asset.getBvhNodes().size();
    }

    bufferUpdated |= vertexBuffers.setElementCount(engine, totalVertices);
    bufferUpdated |= indexBuffers.setElementCount(engine, totalIndices);
    bufferUpdated |= bvhBuffers.setElementCount(engine, totalBvhNodes);

    std::vector<GpuSphere> spheres(sphereBuffers.getCapacity());
    std::vector<GpuPlane> planes(planeBuffers.getCapacity());
    std::vector<GpuBox> boxes(boxBuffers.getCapacity());
    std::vector<Vertex> vertices(vertexBuffers.getCapacity());
    std::vector<uint32_t> indices(indexBuffers.getCapacity());
    std::vector<GpuBvhNode> bvhNodes(bvhBuffers.getCapacity());
    std::vector<GpuMesh> meshes(meshBuffers.getCapacity());
    std::vector<GpuMaterial> materialData(materialBuffers.getCapacity());
    std::vector<ObjectHandle> objectHandles(objectBuffers.getCapacity());
    std::vector<GpuLight> lights;

    int sphereId = 0;
    int planeId = 0;
    int boxId = 0;
    int meshId = 0;
    uint32_t entityCount = 0;
    int lightCount = 0;
    float totalLightArea = 0;
    uint32_t vertexOffset = 0;
    uint32_t indexOffset = 0;
    uint32_t bvhOffset = 0;
    
    std::vector<int> entityToGpuIndex(entities.size(), -1);
    for (size_t i = 0; i < entities.size(); i++) {
        const ecs::Entity& e = entities[i];
        if (!transformStorage.has(e)) continue;
        
        const ecs::Transform& transform = transformStorage.get(e);
        MaterialHandle handle = 0;
        if (materialStorage.has(e)) {
            handle = materialStorage.get(e).handle;
        }

        if (sphereStorage.has(e)) {
            const ecs::Sphere& sphere = sphereStorage.get(e);
            
            spheres[sphereId] = GpuSphere{
                .center = transform.position,
                .radius = sphere.radius,
                .materialHandle = handle,
            };
            
            if (materials[handle].type == MaterialType::Emissive) {
                const float area = 4.0f * glm::pi<float>() * sphere.radius * sphere.radius;
                addLight(materials[spheres[sphereId].materialHandle], area, entityCount, lightCount, lights, totalLightArea);
            }
            objectHandles[entityCount] = { .type=ObjectType::Sphere, .id=sphereId };
            sphereId++;
        } else if (planeStorage.has(e)) {
            planes[planeId] = GpuPlane{
                .point = transform.position,
                .normal = glm::normalize(transform.rotation * glm::vec3(0.0f, 1.0f, 0.0f)),
                .materialHandle = handle,
            };
            objectHandles[entityCount] = { .type=ObjectType::Plane, .id=planeId };
            planeId++;
        } else if (boxStorage.has(e)) {
            boxes[boxId] = GpuBox{
                .transform = transform.local,
                .invTransform = glm::inverse(transform.local),
                .materialHandle = handle,
            };

            if (materials[handle].type == MaterialType::Emissive) {
                const glm::vec3 axisX = glm::vec3(transform.local[0]);
                const glm::vec3 axisY = glm::vec3(transform.local[1]);
                const glm::vec3 axisZ = glm::vec3(transform.local[2]);
                const float hx = glm::length(axisX);
                const float hy = glm::length(axisY);
                const float hz = glm::length(axisZ);
                const float area = 8.0f * (hx * hy + hx * hz + hy * hz);
                addLight(materials[handle], area, entityCount, lightCount, lights, totalLightArea);
            }
            objectHandles[entityCount] = { .type=ObjectType::Box, .id=boxId };
            boxId++;
        } else if (meshStorage.has(e)) {
            const ecs::MeshRef& meshRef = meshStorage.get(e);
            if (meshRef.handle < 0 || static_cast<size_t>(meshRef.handle) >= meshAssets.size())
                continue;
            const MeshAsset& asset = meshAssets[meshRef.handle];
            const auto& meshVerts = asset.getVertices();
            const auto& meshIndices = asset.getIndices();
            const auto& meshBvhNodes = asset.getBvhNodes();

            for (size_t v = 0; v < meshVerts.size(); v++) {
                vertices[vertexOffset + v] = meshVerts[v];
            }
            for (size_t idx = 0; idx < meshIndices.size(); idx++) {
                indices[indexOffset + idx] = meshIndices[idx] + vertexOffset;
            }
            for (size_t n = 0; n < meshBvhNodes.size(); n++) {
                GpuBvhNode node = meshBvhNodes[n];
                if (node.isLeaf != 0) {
                    node.data0 = static_cast<size_t>(node.data0 + (indexOffset / 3));
                } else {
                    node.data0 = static_cast<size_t>(node.data0 + bvhOffset);
                    node.data1 = static_cast<size_t>(node.data1 + bvhOffset);
                }
                bvhNodes[bvhOffset + n] = node;
            }

            meshes[meshId] = GpuMesh{
                .transform = transform.local,
                .invTransform = glm::inverse(transform.local),
                .indexOffset = indexOffset,
                .triangleCount = static_cast<uint32_t>(meshIndices.size() / 3),
                .bvhOffset = bvhOffset,
                .bvhNodeCount = static_cast<uint32_t>(meshBvhNodes.size()),
                .materialHandle = handle,
            };

            objectHandles[entityCount] = { .type=ObjectType::Mesh, .id=meshId };
            vertexOffset += static_cast<uint32_t>(meshVerts.size());
            indexOffset += static_cast<uint32_t>(meshIndices.size());
            bvhOffset += static_cast<uint32_t>(meshBvhNodes.size());
            meshId++;
        } else {
            continue;
        }

        entityToGpuIndex[i] = entityCount;
        entityCount++;
    }
    
    bufferUpdated |= lightBuffers.setElementCount(engine, lightCount);

    for (size_t i = 0; i < materials.size() && i < materialData.size(); i++) {
        materialData[i] = GpuMaterial{
            .type = materials[i].type,
            .albedo = materials[i].albedo,
            .payload = { materials[i].payload[0], materials[i].payload[1] }
        };
    }

    int selected = -1;
    if (selectedEntity >= 0)
        selected = entityToGpuIndex[static_cast<size_t>(selectedEntity)];

    // Fill the buffers
    sphereBuffers.fill(engine, spheres.data());
    planeBuffers.fill(engine, planes.data());
    boxBuffers.fill(engine, boxes.data());
    materialBuffers.fill(engine, materialData.data());
    vertexBuffers.fill(engine, vertices.data());
    indexBuffers.fill(engine, indices.data());
    meshBuffers.fill(engine, meshes.data());
    bvhBuffers.fill(engine, bvhNodes.data());

    size_t offset;
    
    std::vector<char> objectData(OBJECT_HEADER_SIZE + sizeof(ObjectHandle) * objectBuffers.getCapacity(), 0);
    offset = 0;
    memcpy(objectData.data() + offset, &entityCount, sizeof(entityCount));
    offset += sizeof(entityCount);
    memcpy(objectData.data() + offset, &selected, sizeof(selected));
    offset += sizeof(selected);
    if (entityCount > 0)
        memcpy(objectData.data() + offset, objectHandles.data(), objectHandles.size() * sizeof(ObjectHandle));

    objectBuffers.fill(engine, objectData.data());

    std::vector<char> lightData(LIGHT_HEADER_SIZE + sizeof(GpuLight) * lightBuffers.getCapacity(), 0);
    offset = 0;
    memcpy(lightData.data() + offset, &totalLightArea, sizeof(totalLightArea));
    offset += sizeof(totalLightArea);
    memcpy(lightData.data() + offset, lights.data(), lights.size() * sizeof(GpuLight));
        
    lightBuffers.fill(engine, lightData.data());
}


void Scene::drawGuizmo(const glm::mat4 &view, const glm::mat4 &proj) {
    if (selectedEntity < 0) return;

    ecs::Entity e = entities[selectedEntity];
    if (!registry.has<ecs::Transform>(e)) return;

    ecs::Transform& t = registry.get<ecs::Transform>(e);
    t.updateLocal();
    glm::mat4 model = t.local;

    int opFlags = 0;
    if (t.positionToggled) opFlags |= ImGuizmo::OPERATION::TRANSLATE;
    if (t.rotationToggled) opFlags |= ImGuizmo::OPERATION::ROTATE;
    if (t.scaleToggled) opFlags |= ImGuizmo::OPERATION::SCALE;
    if (opFlags == 0) return;
    // Keep gizmo orientation in world space: avoid mixing scale with other ops.
    if ((opFlags & ImGuizmo::OPERATION::SCALE) && (opFlags & (ImGuizmo::OPERATION::TRANSLATE | ImGuizmo::OPERATION::ROTATE)))
    {
        opFlags &= ~ImGuizmo::OPERATION::SCALE;
    }

    ImGuizmo::PushID(selectedEntity);
    if (ImGuizmo::Manipulate(
            glm::value_ptr(view),
            glm::value_ptr(proj),
            static_cast<ImGuizmo::OPERATION>(opFlags),
            ImGuizmo::MODE::WORLD,
            glm::value_ptr(model)))
    {
        glm::vec3 translation, rotationEuler, scale;
        ImGuizmo::DecomposeMatrixToComponents(
            glm::value_ptr(model),
            glm::value_ptr(translation),
            glm::value_ptr(rotationEuler),
            glm::value_ptr(scale));

        if (t.positionToggled) t.setPosition(translation);
        if (t.rotationToggled) t.setRotation(glm::quat(glm::radians(rotationEuler)));
        if (t.scaleToggled) t.setScale(scale);
        updated = true;
    }
    ImGuizmo::PopID();

    t.updateLocal();
}

void Scene::drawUI() {
    assert(ctx);
    bool openNewObjectPopup = false;
    bool openNewMeshAssetPopup = false;

    // Draw entity list
    if (ImGui::BeginTable("Entities", 2, ImGuiTableFlags_None)) {
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Entities");
        if (ImGui::BeginListBox("##Entities", ImVec2(-FLT_MIN, 0.0f))) {
            for (size_t i = 0; i < entities.size(); i++) {
                const ecs::Entity& e = entities[i];

                std::string displayName = "???";
                if (registry.has<ecs::Name>(e)) displayName = registry.get<ecs::Name>(e).value;
                
                if (registry.has<ecs::EditorOnly>(e)) {
                    ImGui::TextDisabled("%s", displayName.c_str());
                } else {
                    if (ImGui::Selectable(displayName.c_str(), selectedEntity == i, ImGuiSelectableFlags_AllowDoubleClick)) {
                        if (ImGui::IsMouseDoubleClicked(0)) { selectedEntity = i; }
                    }
                }
            }

            ImGui::EndListBox();
        }
        
        ImGui::TableSetColumnIndex(1);
        ImGui::NewLine();
        if (ImGui::Button("+##Entity", ImVec2(32, 0)))
            openNewObjectPopup = true;
        if (ImGui::Button("-##Entity", ImVec2(32, 0)) && selectedEntity >= 0) {
            ecs::Entity e = entities[static_cast<size_t>(selectedEntity)];
            registry.destroyEntity(e);
            entities.erase(std::next(entities.begin(), selectedEntity));
            selectedEntity = -1;
            updated = true;
            bufferUpdated = true;
        }

        ImGui::EndTable();
    }

    // Draw material list
    if (ImGui::BeginTable("Materials", 2, ImGuiTableFlags_None)) {
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Materials");
        if (ImGui::BeginListBox("##Materials", ImVec2(-FLT_MIN, 0.0f))) {
            for (size_t i = 0; i < materials.size(); i++) {
                const std::string& materialName = materials[i].name;
                const char* display = materialName.empty() ? "Material" : materialName.c_str();
                std::string label = std::string(display) + "##Material" + std::to_string(i);
                if (ImGui::Selectable(label.c_str(), selectedMaterial == i, ImGuiSelectableFlags_AllowDoubleClick)) {
                    if (ImGui::IsMouseDoubleClicked(0)) { selectedMaterial = i; }
                }

            }

            ImGui::EndListBox();
        }
        
        ImGui::TableSetColumnIndex(1);
        ImGui::NewLine();
        if (ImGui::Button("+##Materials", ImVec2(32, 0))) {
            Material mat = DEFAULT_MATERIAL;
            char matName[64];
            std::snprintf(matName, sizeof(matName), "Material-%02d", materialN++);
            mat.name = matName;
            selectedMaterial = pushMaterial(mat);
            updated = true;
            bufferUpdated = true;
        }
        if (ImGui::Button("-##Materials", ImVec2(32, 0)) &&
            selectedMaterial > 0 &&
            selectedMaterial < static_cast<int>(materials.size()))
        {
            const int removed = selectedMaterial;
            materials.erase(materials.begin() + removed);
            selectedMaterial = -1;

            auto& matRefs = registry.storage<ecs::MaterialRef>();
            const auto& refEntities = matRefs.entities();
            for (size_t i = 0; i < matRefs.size(); i++) {
                ecs::MaterialRef& ref = matRefs.get(refEntities[i]);
                if (ref.handle == removed)
                    ref.handle = 0;
                else if (ref.handle > removed)
                    ref.handle--;
            }
            updated = true;
            bufferUpdated = true;
        }

        ImGui::EndTable();
    }

    // Draw mesh asset list
    if (ImGui::BeginTable("MeshAssets", 2, ImGuiTableFlags_None)) {
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Mesh Assets");
        if (ImGui::BeginListBox("##MeshAssets", ImVec2(-FLT_MIN, 0.0f))) {
            for (size_t i = 0; i < meshAssets.size(); i++) {
                const std::string& meshName = meshAssets[i].getName();
                const char* display = meshName.empty() ? "Mesh" : meshName.c_str();
                std::string label = std::string(display) + "##MeshAsset" + std::to_string(i);
                if (ImGui::Selectable(label.c_str(), selectedMeshAsset == static_cast<int>(i), ImGuiSelectableFlags_AllowDoubleClick)) {
                    if (ImGui::IsMouseDoubleClicked(0)) { selectedMeshAsset = static_cast<int>(i); }
                }
            }
            ImGui::EndListBox();
        }

        ImGui::TableSetColumnIndex(1);
        ImGui::NewLine();
        if (ImGui::Button("+##MeshAssets", ImVec2(32, 0)))
            openNewMeshAssetPopup = true;
        if (ImGui::Button("-##MeshAssets", ImVec2(32, 0)) && selectedMeshAsset > 0 && selectedMeshAsset < static_cast<int>(meshAssets.size())) {
            const int removed = selectedMeshAsset;
            meshAssets.erase(meshAssets.begin() + removed);
            selectedMeshAsset = -1;

            auto& meshRefs = registry.storage<ecs::MeshRef>();
            const auto& refEntities = meshRefs.entities();
            for (size_t i = 0; i < meshRefs.size(); i++) {
                ecs::MeshRef& ref = meshRefs.get(refEntities[i]);
                if (ref.handle == removed)
                    ref.handle = 0;
                else if (ref.handle > removed)
                    ref.handle--;
            }
            updated = true;
            bufferUpdated = true;
        }

        ImGui::EndTable();
    }

    if (openNewMeshAssetPopup) ImGui::OpenPopup("New Mesh Asset");
    if (ImGui::BeginPopupModal("New Mesh Asset", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        static char meshPath[256] = "";
        ImGui::Text("Path:");
        ImGui::PushItemWidth(420.0f);
        ImGui::InputText("##MeshPath", meshPath, sizeof(meshPath));
        ImGui::PopItemWidth();

        bool hasPath = std::strlen(meshPath) > 0;
        ImGui::BeginDisabled(!hasPath);
        if (ImGui::Button("Load", ImVec2(100, 0))) {
            MeshAsset asset(MeshAsset::nameFromPath(meshPath));
            if (asset.loadFromObj(*ctx, meshPath)) {
                meshAssets.push_back(std::move(asset));
                selectedMeshAsset = static_cast<int>(meshAssets.size()) - 1;
                updated = true;
                bufferUpdated = true;
                meshPath[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (openNewObjectPopup) ImGui::OpenPopup("New Object");
    drawNewObjectPopUp();
}

void Scene::drawNewObjectPopUp() {
    if (!ImGui::BeginPopupModal("New Object", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) return;
    
    char nameBuffer[64];
    std::snprintf(nameBuffer, sizeof(nameBuffer), "Entity-%02d", entityN);
    std::string name(nameBuffer);

    if (ImGui::Button("Sphere", { 100, 0 })) {
        pushSphere(
            name,
            glm::vec3(0.0, 0.0, 0.0),
            1.0,
            0
        );
        selectedEntity = entities.size() - 1;
        updated = true;
        ImGui::CloseCurrentPopup();
        entityN++;
    }
    if (ImGui::Button("Plane", { 100, 0 })) {
        pushPlane(
            name,
            glm::vec3( 0.0, 0.0, 0.0),
            glm::vec3( 0.0, 1.0, 0.0),
            0
        );
        selectedEntity = entities.size() - 1;
        updated = true;
        ImGui::CloseCurrentPopup();
        entityN++;
    }
    if (ImGui::Button("Box", { 100, 0 })) {
        pushBox(
            name,
            glm::vec3(-1.0,-1.0,-1.0),
            glm::vec3( 1.0, 1.0, 1.0),
            0
        );
        selectedEntity = entities.size() - 1;
        updated = true;
        ImGui::CloseCurrentPopup();
        entityN++;
    }
    if (ImGui::Button("Mesh", { 100, 0 })) {
        pushMesh(
            name,
            0,
            glm::mat3(1.0),
            0
        );
        selectedEntity = entities.size() - 1;
        updated = true;
        ImGui::CloseCurrentPopup();
        entityN++;
    }
    if (ImGui::Button("Camera", { 100, 0 })) {
        glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 direction = glm::vec3(0.0f, 0.0f, 1.0f);
        pushCameraHandle(
            name,
            pos,
            direction,
            60.0f
        );
        updated = true;
        ImGui::CloseCurrentPopup();
        entityN++;
    }
    ImGui::PushStyleColor(ImGuiCol_Button, { 1.0, 0.03, 0.0, 1.0 });
    if (ImGui::Button("Cancel", { 100, 0 })) {
        ImGui::CloseCurrentPopup();
    }
    ImGui::PopStyleColor();

    ImGui::EndPopup();
}

void Scene::drawSelectedEntityUI() {
    if (selectedEntity < 0) return;
    ecs::Entity& e = entities[selectedEntity];

    bool open = true;
    ImGui::SetNextWindowBgAlpha(0.8f);
    ImGui::SetNextWindowSizeConstraints({ 250.0f, 0.0f }, { 250.0f, 600.0f });
    ImGui::Begin(
        "Entity",
        &open,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing
    );
    {
        auto& ui_reg = ecs::ComponentUiRegistry::get();
        updated |= ui_reg.draw(registry, e);
    }
    ImGui::End();

    if (!open) selectedEntity = -1;
}

void Scene::drawSelectedMaterialUI() {
    if (selectedMaterial < 0) return;

    bool open = true;
    ImGui::SetNextWindowBgAlpha(0.8f);
    ImGui::SetNextWindowSizeConstraints({ 250.0f, 0.0f }, { 250.0f, 600.0f });
    ImGui::Begin("Material", &open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing);
    {
        auto& mat = materials[selectedMaterial];
        updated |= drawMaterialUI(mat);
    }
    ImGui::End();

    if (!open) selectedMaterial = -1;
}

void Scene::drawSelectedMeshAssetUI() {
     if (selectedMeshAsset < 0) return;

    bool open = true;
    ImGui::SetNextWindowBgAlpha(0.8f);
    ImGui::SetNextWindowSizeConstraints({ 250.0f, 0.0f }, { 250.0f, 600.0f });
    ImGui::Begin("Mesh Asset", &open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing);
    {
        updated |= drawMeshAssetUI(meshAssets[selectedMeshAsset]);
    }
    ImGui::End();

    if (!open) selectedMeshAsset = -1;
}


bool Scene::raycast(const glm::vec2 &screenPos, const glm::vec2 &screenSize, const Camera &camera, float &dist, glm::vec3 &p, bool select, bool includeCameras) {
    const Ray ray = getRay(screenPos, screenSize, camera);
    float tClosest = std::numeric_limits<float>::infinity();
    int idClosest = -1;

    float t = -1.0f;

    auto& sphereStorage = registry.storage<ecs::Sphere>();
    auto& planeStorage = registry.storage<ecs::Plane>();
    auto& boxStorage = registry.storage<ecs::Box>();
    auto& meshStorage = registry.storage<ecs::MeshRef>();
    auto& transformStorage = registry.storage<ecs::Transform>();
    auto& selectableStorage = registry.storage<ecs::Selectable>();

    for (size_t i = 0; i < entities.size(); i++) {
        const ecs::Entity& e = entities[i];

        if (!selectableStorage.has(e) || !transformStorage.has(e)) continue;

        auto& transform = transformStorage.get(e);
        
        if (sphereStorage.has(e)) {
            auto& sphere = sphereStorage.get(e);
            t = raySphereIntersection(ray, transform.position, sphere.radius);
        } else if (planeStorage.has(e)) {
            glm::vec3 normal = glm::normalize(transform.rotation * glm::vec3(0.0f, 1.0f, 0.0f));
            t = rayPlaneIntersection(ray, transform.position, normal);
        } else if (boxStorage.has(e)) {
            t = rayBoxIntersection(ray, transform.local);
        } else if (meshStorage.has(e)) {
            const ecs::MeshRef& meshRef = meshStorage.get(e);
            if (meshRef.handle >= 0 && static_cast<size_t>(meshRef.handle) < meshAssets.size()) {
                const MeshAsset& asset = meshAssets[meshRef.handle];
                t = rayMeshIntersection(ray, transform.local, asset.getVertices(), asset.getIndices());
            }
        }

        if (t >= 0.0f && t < tClosest) {
            tClosest = t;
            idClosest = i;
        }
    }
    
    if (select) selectedEntity = idClosest;
    dist = tClosest;
    p = ray.origin + dist * ray.dir;
    return idClosest >= 0;
}


bool Scene::containsObject(const Object *object) const {
    for (const Object *candidate : objects) {
        if (candidate == object)
            return true;
    }
    return false;
}

std::vector<bufferList_t> Scene::getBufferLists() {
    std::vector<bufferList_t> bufferLists = {
        sphereBuffers.getBufferList(),
        planeBuffers.getBufferList(),
        boxBuffers.getBufferList(),
        vertexBuffers.getBufferList(),
        indexBuffers.getBufferList(),
        bvhBuffers.getBufferList(),
        meshBuffers.getBufferList(),
        materialBuffers.getBufferList(),
        objectBuffers.getBufferList(),
        lightBuffers.getBufferList(),
    };


    return bufferLists;
}

bool Scene::checkUpdate() {
    if (updated) {
        updated = false;
        return true;
    }
    return false;
}

bool Scene::checkBufferUpdate() {
    if (bufferUpdated) {
        bufferUpdated = false;
        return true;
    }
    return false;
}
