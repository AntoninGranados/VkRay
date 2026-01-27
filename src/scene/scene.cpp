// 000101001
// 001000000

#include "scene.hpp"
#include "scene_preset.hpp"

#include <utility>
#include <iostream>
#include <cstring>
#include <cstdio>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

constexpr size_t OBJECT_HEADER_SIZE = sizeof(unsigned int) + sizeof(int);
constexpr size_t LIGHT_HEADER_SIZE = sizeof(float);

void Scene::init(VkSmol &engine) {
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
    ecs::ComponentUiRegistry::init();

    materials.push_back(DEFAULT_MATERIAL);
}

void Scene::destroy(VkSmol &engine) {
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

void Scene::clear(VkSmol &engine) {
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
    selectedEntity = -1;
    bufferUpdated = true;

    materials.push_back(DEFAULT_MATERIAL);
}

LightMode Scene::loadPreset(VkSmol &engine, ScenePreset preset) {
    LightMode mode = LightMode::Day;
    switch (preset) {
        case ScenePreset::Empty:
            initEmpty(engine, *this, mode);
            break;
        case ScenePreset::Mesh:
            initMesh(engine, *this, mode);
            break;
        case ScenePreset::Sponza:
            initSponza(engine, *this, mode);
            break;
        case ScenePreset::CornellBox:
            initCornellBox(engine, *this, mode);
            break;
        case ScenePreset::RandomSpheres:
            initRandomSpheres(engine, *this, mode);
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


void Scene::pushSphere(VkSmol &engine, std::string name, glm::vec3 center, float radius, Material mat) {
    bufferUpdated |= sphereBuffers.addElement(engine);
    bufferUpdated |= materialBuffers.addElement(engine);
    bufferUpdated |= objectBuffers.addElement(engine);

    // const MaterialHandle materialHandle = static_cast<int>(materials.size());
    // materials.push_back(mat);

    ecs::Entity e = registry.createEntity();
    
    registry.add<ecs::Selectable>(e, ecs::Selectable{});

    ecs::Name nameComponent;
    nameComponent.setValue(std::move(name));
    registry.add<ecs::Name>(e, nameComponent);

    ecs::Sphere sphereComponent;
    sphereComponent.setRadius(radius);
    registry.add<ecs::Sphere>(e, sphereComponent);

    ecs::MaterialRef materialRefComponent;
    materialRefComponent.setHandle(MaterialHandle(0));
    registry.add<ecs::MaterialRef>(e, materialRefComponent);
    
    ecs::Transform transformComponent;
    transformComponent.setPosition(center);
    transformComponent.setRotationToggle(false);
    transformComponent.setScaleToggle(false);
    registry.add<ecs::Transform>(e, transformComponent);

    entities.push_back(e);
}

void Scene::pushPlane(VkSmol &engine, std::string name, glm::vec3 point, glm::vec3 normal, Material mat) {
    bufferUpdated |= planeBuffers.addElement(engine);
    bufferUpdated |= materialBuffers.addElement(engine);
    bufferUpdated |= objectBuffers.addElement(engine);

    // const MaterialHandle materialHandle = static_cast<int>(materials.size());
    // materials.push_back(mat);

    ecs::Entity e = registry.createEntity();
    
    registry.add<ecs::Selectable>(e, ecs::Selectable{});

    ecs::Name nameComponent;
    nameComponent.setValue(std::move(name));
    registry.add<ecs::Name>(e, nameComponent);

    registry.add<ecs::Plane>(e, ecs::Plane{});

    ecs::MaterialRef materialRefComponent;
    materialRefComponent.setHandle(MaterialHandle(0));
    registry.add<ecs::MaterialRef>(e, materialRefComponent);
    
    ecs::Transform transformComponent;
    transformComponent.setPosition(point);
    transformComponent.setRotation(glm::rotation(glm::vec3(0.0f, 1.0f, 0.0f), normal));
    transformComponent.setScaleToggle(false);
    registry.add<ecs::Transform>(e, transformComponent);

    entities.push_back(e);
}

void Scene::pushBox(VkSmol &engine, std::string name, glm::vec3 cornerMin, glm::vec3 cornerMax, Material mat) {
    glm::vec3 center = (cornerMin + cornerMax) * 0.5f;
    glm::vec3 halfExtents = (cornerMax - cornerMin) * 0.5f;
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), center);
    transform = glm::scale(transform, halfExtents);
    
    bufferUpdated |= boxBuffers.addElement(engine);
    bufferUpdated |= materialBuffers.addElement(engine);
    bufferUpdated |= objectBuffers.addElement(engine);

    // const MaterialHandle materialHandle = static_cast<int>(materials.size());
    // materials.push_back(mat);

    ecs::Entity e = registry.createEntity();
    
    registry.add<ecs::Selectable>(e, ecs::Selectable{});

    ecs::Name nameComponent;
    nameComponent.setValue(std::move(name));
    registry.add<ecs::Name>(e, nameComponent);

    registry.add<ecs::Box>(e, ecs::Box{});

    ecs::MaterialRef materialRefComponent;
    materialRefComponent.setHandle(MaterialHandle(0));
    registry.add<ecs::MaterialRef>(e, materialRefComponent);
    
    ecs::Transform transformComponent;
    transformComponent.setPosition(center);
    transformComponent.setScale(halfExtents);
    transformComponent.updateLocal();
    registry.add<ecs::Transform>(e, transformComponent);

    entities.push_back(e);
}

void Scene::pushMesh(VkSmol &engine, std::string name, std::vector<Vertex> vertices, std::vector<unsigned int> indices, glm::mat4 transform, Material mat) {
    return;

    bufferUpdated |= meshBuffers.addElement(engine);
    bufferUpdated |= materialBuffers.addElement(engine);
    bufferUpdated |= objectBuffers.addElement(engine);

    objects.push_back(new Mesh(name, std::move(vertices), std::move(indices), transform, static_cast<int>(materials.size())));
    materials.push_back(mat);
}

void Scene::pushCameraHandle(std::string name, glm::vec3 position, glm::vec3 direction, float fov) {
    CameraHandle *cameraHandle = new CameraHandle(name, position, direction, fov);
    if (previewCameraCallback)
        cameraHandle->setPreviewCallback(previewCameraCallback);
    objects.push_back(cameraHandle);
    updated = true;
}

bool Scene::pushMeshFromObj(VkSmol &engine, const std::string &name, const std::string &path, Material mat, const glm::mat4 &transform) {
    return true;
    
    std::string baseDir = "./";
    size_t slash = path.find_last_of("/\\");
    if (slash != std::string::npos) {
        baseDir = path.substr(0, slash + 1);
    }

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;
    bool loaded = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), baseDir.c_str(), true);
    if (!warn.empty()) {
        std::cerr << "[WARN] OBJ load: " << warn << std::endl;
    }
    if (!err.empty()) {
        std::cerr << "[ERROR] OBJ load: " << err << std::endl;
    }
    if (!loaded) {
        std::cerr << "[ERROR] Failed to load OBJ: " << path << std::endl;
        return false;
    }

    std::vector<Vertex> meshVertices;
    meshVertices.reserve(attrib.vertices.size() / 3);
    for (size_t i = 0; i + 2 < attrib.vertices.size(); i += 3) {
        meshVertices.push_back(Vertex{
            .position = glm::vec3(attrib.vertices[i + 0], attrib.vertices[i + 1], attrib.vertices[i + 2]),
        });
    }

    std::vector<unsigned int> meshIndices;
    for (const tinyobj::shape_t &shape : shapes) {
        size_t indexOffset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            int fv = shape.mesh.num_face_vertices[f];
            if (fv != 3) {
                indexOffset += fv;
                continue;
            }

            for (int v = 0; v < fv; v++) {
                const tinyobj::index_t idx = shape.mesh.indices[indexOffset + v];
                if (idx.vertex_index < 0) continue;
                meshIndices.push_back(static_cast<unsigned int>(idx.vertex_index));
            }
            indexOffset += fv;
        }
    }

    pushMesh(engine, name, std::move(meshVertices), std::move(meshIndices), transform, mat);
    return true;
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
void Scene::fillBuffers(VkSmol &engine) {
    size_t totalVertices = 0;
    size_t totalIndices = 0;
    size_t totalBvhNodes = 0;
    for (size_t objIndex = 0; objIndex < objects.size(); objIndex++) {
        Object *object = objects[objIndex];
        if (object->getType() == ObjectType::Mesh) {
            Mesh *mesh = static_cast<Mesh*>(object);
            totalVertices += mesh->getVertices().size();
            totalIndices += mesh->getIndices().size();
            totalBvhNodes += mesh->getBvhNodes().size();
        }
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
    // uint32_t objectCount = 0;
    uint32_t entityCount = 0;
    int lightCount = 0;
    float totalLightArea = 0;
    uint32_t vertexOffset = 0;
    uint32_t indexOffset = 0;
    uint32_t bvhOffset = 0;
    
    std::vector<int> entityToGpuIndex(entities.size(), -1);

    const auto& sphereStorage = registry.storage<ecs::Sphere>();
    const auto& planeStorage = registry.storage<ecs::Plane>();
    const auto& boxStorage = registry.storage<ecs::Box>();
    const auto& transformStorage = registry.storage<ecs::Transform>();
    const auto& materialStorage = registry.storage<ecs::MaterialRef>();
    
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
            .payload = { *materials[i].payload }
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
    
    uint32_t objectCount = entityCount;
    std::vector<char> objectData(OBJECT_HEADER_SIZE + sizeof(ObjectHandle) * objectBuffers.getCapacity(), 0);
    offset = 0;
    memcpy(objectData.data() + offset, &objectCount, sizeof(objectCount));
    offset += sizeof(objectCount);
    memcpy(objectData.data() + offset, &selected, sizeof(selected));
    offset += sizeof(selected);
    if (objectCount > 0)
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

void Scene::drawUI(VkSmol &engine) {
    bool openNewObjectPopup = false;

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
                if (ImGui::Selectable(materials[i].name.c_str(), selectedMaterial == i, ImGuiSelectableFlags_AllowDoubleClick)) {
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
            materials.push_back(mat);
            selectedMaterial = static_cast<int>(materials.size() - 1);
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

    if (openNewObjectPopup) ImGui::OpenPopup("New Object");
    drawNewObjectPopUp(engine);
}

void Scene::drawNewObjectPopUp(VkSmol &engine) {
    if (!ImGui::BeginPopupModal("New Object", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) return;
    
    char nameBuffer[64];
    std::snprintf(nameBuffer, sizeof(nameBuffer), "Entity-%02d", entityN);
    std::string name(nameBuffer);

    if (ImGui::Button("Sphere", { 100, 0 })) {
        pushSphere(
            engine,
            name,
            glm::vec3(0.0, 0.0, 0.0),
            1.0,
            Material {
                .type = Lambertian,
                .albedo = { 1.0, 0.0, 1.0 },
            }
        );
        selectedEntity = entities.size() - 1;
        updated = true;
        ImGui::CloseCurrentPopup();
        entityN++;
    }
    if (ImGui::Button("Plane", { 100, 0 })) {
        pushPlane(
            engine,
            name,
            glm::vec3( 0.0, 0.0, 0.0),
            glm::vec3( 0.0, 1.0, 0.0),
            Material {
                .type = Lambertian,
                .albedo = { 1.0, 0.0, 1.0 },
            }
        );
        selectedEntity = entities.size() - 1;
        updated = true;
        ImGui::CloseCurrentPopup();
        entityN++;
    }
    if (ImGui::Button("Box", { 100, 0 })) {
        pushBox(
            engine,
            name,
            glm::vec3(-1.0,-1.0,-1.0),
            glm::vec3( 1.0, 1.0, 1.0),
            Material {
                .type = Lambertian,
                .albedo = { 1.0, 0.0, 1.0 },
            }
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


bool Scene::raycast(const glm::vec2 &screenPos, const glm::vec2 &screenSize, const Camera &camera, float &dist, glm::vec3 &p, bool select, bool includeCameras) {
    const Ray ray = getRay(screenPos, screenSize, camera);
    float tClosest = std::numeric_limits<float>::infinity();
    int idClosest = -1;

    float t = -1.0f;

    auto& sphereStorage = registry.storage<ecs::Sphere>();
    auto& planeStorage = registry.storage<ecs::Plane>();
    auto& boxStorage = registry.storage<ecs::Box>();
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
