#include "scene.hpp"
#include "scene_preset.hpp"

#include <utility>
#include <iostream>
#include <cstring>
#include <cstdio>
#include <cassert>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "IconsFontAwesome7.h"
#include "../ui_constants.hpp"

#include "../ecs/systems/transform_system.hpp"
#include "../ecs/systems/gpu_packing_system.hpp"
#include "../ecs/systems/camera_system.hpp"
#include "../ecs/systems/animation_system.hpp"
#include "../notification_handler.hpp"

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

    initSystems();

    pushMaterial(DEFAULT_MATERIAL);
    meshAssets.push_back(DEFAULT_MESH_ASSET);
}

void Scene::initSystems() {
    preUpdateScheduler.clear();
    preUpdateScheduler.add(ecs::transformAnimationSystem);
    preUpdateScheduler.add(ecs::transformSystem);
    preUpdateScheduler.add(ecs::cameraPreUpdateSystem);

    onRenderScheduler.clear();
    onRenderScheduler.add(ecs::spherePackingSystem);
    onRenderScheduler.add(ecs::planePackingSystem);
    onRenderScheduler.add(ecs::boxPackingSystem);
    onRenderScheduler.add(ecs::meshPackingSystem);
    onRenderScheduler.add(ecs::materialPackingSystem);
    onRenderScheduler.add(ecs::objectPackingSystem);
    onRenderScheduler.add(ecs::lightPackingSystem);

    
    onUiScheduler.clear();
    onUiScheduler.add(ecs::cameraDrawingSystem);
    
    postUpdateScheduler.clear();
    postUpdateScheduler.add(ecs::cameraPostUpdateSystem);
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

    for (auto& e : entities) { registry.destroyEntity(e); }
    entities.clear();
    entityN = 0;
    materials.clear();
    materialN = 0;
    meshAssets.clear();
    selectedEntity = -1;
    selectedMaterial = -1;
    selectedMeshAsset = -1;
    bufferUpdated = true;

    pushMaterial(DEFAULT_MATERIAL);
    meshAssets.push_back(DEFAULT_MESH_ASSET);
}

LightMode Scene::loadPreset(ScenePreset preset) {
    LightMode mode = LightMode::Day;
    switch (preset) {
        case ScenePreset::Empty:         initEmpty(*this, mode);         break;
        case ScenePreset::Mesh:          initMesh(*this, mode);          break;
        case ScenePreset::Sponza:        initSponza(*this, mode);        break;
        case ScenePreset::CornellBox:    initCornellBox(*this, mode);    break;
        case ScenePreset::RandomSpheres: initRandomSpheres(*this, mode); break;
    }
    return mode;
}

MaterialHandle Scene::pushMaterial(const Material &mat) {
    const MaterialHandle materialHandle = static_cast<int>(materials.size());
    materials.push_back(mat);
    return materialHandle;
}

void Scene::pushSphere(std::string name, glm::vec3 center, float radius, MaterialHandle materialHandle) {
    ecs::Entity e = registry.createEntity();

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
    registry.add<ecs::Transform>(e, transformComponent);

    entities.push_back(e);
}

void Scene::pushPlane(std::string name, glm::vec3 point, glm::vec3 normal, MaterialHandle materialHandle) {
    ecs::Entity e = registry.createEntity();

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
    registry.add<ecs::Transform>(e, transformComponent);

    entities.push_back(e);
}

void Scene::pushBox(std::string name, glm::vec3 cornerMin, glm::vec3 cornerMax, MaterialHandle materialHandle) {
    glm::vec3 center = (cornerMin + cornerMax) * 0.5f;
    glm::vec3 halfExtents = (cornerMax - cornerMin) * 0.5f;
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), center);
    transform = glm::scale(transform, halfExtents);

    ecs::Entity e = registry.createEntity();

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
    ecs::Entity e = registry.createEntity();

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
    registry.add<ecs::Transform>(e, transformComponent);

    entities.push_back(e);
}

void Scene::pushCamera(std::string name, const glm::mat4 &transform) {
    glm::vec3 translation, rotationEuler, scale;
    ImGuizmo::DecomposeMatrixToComponents(
        glm::value_ptr(transform),
        glm::value_ptr(translation),
        glm::value_ptr(rotationEuler),
        glm::value_ptr(scale));

    ecs::Entity e = registry.createEntity();

    ecs::Name nameComponent;
    nameComponent.setValue(std::move(name));
    registry.add<ecs::Name>(e, nameComponent);
    
    ecs::CameraObject cameraComponent;
    cameraComponent.setFov(60.0f);
    cameraComponent.setAperture(0.0f);
    cameraComponent.setFocusDepth(1.0f);
    registry.add<ecs::CameraObject>(e, cameraComponent);

    
    ecs::Transform transformComponent;
    transformComponent.setPosition(translation);
    transformComponent.setRotation(glm::quat(rotationEuler));
    registry.add<ecs::Transform>(e, transformComponent);

    entities.push_back(e);
}


void Scene::drawGuizmo(const glm::mat4 &view, const glm::mat4 &proj) {
    if (selectedEntity < 0) return;

    ecs::Entity e = entities[selectedEntity];
    if (!registry.has<ecs::Transform>(e)) return;

    ecs::Transform& t = registry.get<ecs::Transform>(e);
    glm::mat4 model = t.local;

    int opFlags = ImGuizmo::OPERATION::TRANSLATE | ImGuizmo::OPERATION::ROTATE | ImGuizmo::OPERATION::SCALE;
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
        if (isInvalid(model)) {
            ImGuizmo::PopID();
            return;
        }

        glm::vec3 translation, rotationEuler, scale;
        ImGuizmo::DecomposeMatrixToComponents(
            glm::value_ptr(model),
            glm::value_ptr(translation),
            glm::value_ptr(rotationEuler),
            glm::value_ptr(scale));

        t.setPosition(translation);
        t.setRotation(glm::quat(glm::radians(rotationEuler)));
        t.setScale(scale);
        updated = true;
    }
    ImGuizmo::PopID();
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
                if (displayName.empty()) displayName = "???";
                
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
            updated = true;
            bufferUpdated = true;
            selectedEntity = -1;
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
            pushMaterial(mat);
            updated = true;
            bufferUpdated = true;
        }
        if (ImGui::Button("-##Materials", ImVec2(32, 0)) &&
            selectedMaterial > 0 &&
            selectedMaterial < static_cast<int>(materials.size()))
        {
            const int removed = selectedMaterial;
            materials.erase(materials.begin() + removed);

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
            selectedMaterial = -1;
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
            selectedMeshAsset = -1;
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
        if (ImGui::Button(ICON_FA_UPLOAD " Load", ui::button_size)) {
            MeshAsset asset(MeshAsset::nameFromPath(meshPath));
            if (asset.loadFromObj(*ctx, meshPath)) {
                meshAssets.push_back(std::move(asset));
                updated = true;
                bufferUpdated = true;
                meshPath[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        ui::PushCancelStyleColor();
        if (ImGui::Button(ICON_FA_BAN " Cancel", ui::button_size)) {
            ImGui::CloseCurrentPopup();
        }
        ui::PopCancelStyleColor();
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

    if (ImGui::Button(ICON_FA_BORDER_NONE " Empty", ui::button_size)) {
        entities.push_back(registry.createEntity());
        updated = true;
        ImGui::CloseCurrentPopup();
        entityN++;
    }
    if (ImGui::Button(ICON_FA_CIRCLE " Sphere", ui::button_size)) {
        pushSphere(name, glm::vec3(0.0, 0.0, 0.0), 1.0);
        updated = true;
        ImGui::CloseCurrentPopup();
        entityN++;
    }
    if (ImGui::Button(ICON_FA_SQUARE " Plane", ui::button_size)) {
        pushPlane(name, glm::vec3( 0.0, 0.0, 0.0), glm::vec3( 0.0, 1.0, 0.0));
        updated = true;
        ImGui::CloseCurrentPopup();
        entityN++;
    }
    if (ImGui::Button(ICON_FA_BOX " Box", ui::button_size)) {
        pushBox(name, glm::vec3(-1.0,-1.0,-1.0), glm::vec3( 1.0, 1.0, 1.0));
        updated = true;
        ImGui::CloseCurrentPopup();
        entityN++;
    }
    if (ImGui::Button(ICON_FA_CUBE " Mesh", ui::button_size)) {
        pushMesh(name, 0, glm::mat3(1.0));
        updated = true;
        ImGui::CloseCurrentPopup();
        entityN++;
    }
    if (ImGui::Button(ICON_FA_VIDEO " Camera", ui::button_size)) {
        pushCamera(name, glm::mat3(1.0));
        // updated = true;
        ImGui::CloseCurrentPopup();
        entityN++;
    }
    ui::PushCancelStyleColor();
    if (ImGui::Button(ICON_FA_BAN " Cancel", ui::button_size)) {
        ImGui::CloseCurrentPopup();
    }
    ui::PopCancelStyleColor();

    ImGui::EndPopup();
}

void Scene::drawSelectedEntityUI() {
    if (selectedEntity < 0) return;
    ecs::Entity& e = entities[selectedEntity];
    bool openNewComponentPopup = false;

    bool open = true;
    ImGui::SetNextWindowBgAlpha(ui::window_bg_alpha);
    ImGui::SetNextWindowSizeConstraints({ 250.0f, 0.0f }, { 250.0f, 600.0f });
    ImGui::Begin(
        "Entity",
        &open,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing
    );
    {   
        ImGui::Text("Add Component");
        ImGui::SameLine();
        if (ImGui::Button("+##AddComponent", {32, 0})) {
            openNewComponentPopup = true;
        }
        
        auto& ui_reg = ecs::ComponentUiRegistry::get();
        updated |= ui_reg.draw(*ctx, registry, e);
    }
    ImGui::End();

    if (openNewComponentPopup) ImGui::OpenPopup("Add Component");
    if (ImGui::BeginPopupModal("Add Component", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        for (auto& [id, funcs] : componentFuncs()) {
            if (ImGui::Button(componentLabel(id).c_str(), ui::button_size)) {
                bool verifyRestrictions = true;
                const auto& restrictions = componentRestrictions()[id];
                for (auto& requirement : restrictions.requirements) {
                    if (!componentFuncs()[requirement].has(registry, e)) {
                        verifyRestrictions = false;
                        ctx->notifications->pushMessage(NotificationType::Warning, "Missing component " + componentLabel(requirement));
                    }
                }
                for (auto& conflict : restrictions.conflicts) {
                    if (componentFuncs()[conflict].has(registry, e)) {
                        verifyRestrictions = false;
                        ctx->notifications->pushMessage(NotificationType::Warning, "Conflicting component " + componentLabel(conflict));
                    }
                }

                if (!verifyRestrictions) {
                    ctx->notifications->pushMessage(NotificationType::Error, "Failed to add component, not all restrictions met");
                } else {
                    funcs.add(registry, e);
                    *ctx->restartRender = true;
                }
                ImGui::CloseCurrentPopup();
            }
        }

        ui::PushCancelStyleColor();
        if (ImGui::Button(ICON_FA_BAN " Cancel", ui::button_size)) {
            ImGui::CloseCurrentPopup();
        }
        ui::PopCancelStyleColor();
        ImGui::EndPopup();
    }

    if (!open) selectedEntity = -1;
}

void Scene::drawSelectedMaterialUI() {
    if (selectedMaterial < 0) return;

    bool open = true;
    ImGui::SetNextWindowBgAlpha(ui::window_bg_alpha);
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
    ImGui::SetNextWindowBgAlpha(ui::window_bg_alpha);
    ImGui::SetNextWindowSizeConstraints({ 250.0f, 0.0f }, { 250.0f, 600.0f });
    ImGui::Begin("Mesh Asset", &open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing);
    {
        bool changed = drawMeshAssetUI(meshAssets[selectedMeshAsset]);
        updated |= changed;
        bufferUpdated |= changed;
    }
    ImGui::End();

    if (!open) selectedMeshAsset = -1;
}


bool Scene::raycast(const glm::vec2 &screenPos, const glm::vec2 &screenSize, float &dist, glm::vec3 &p, bool select, bool includeCameras) {
    const Ray ray = getRay(screenPos, screenSize, camera);
    float tClosest = std::numeric_limits<float>::infinity();
    int idClosest = -1;

    float t = -1.0f;

    auto& sphereStorage = registry.storage<ecs::Sphere>();
    auto& planeStorage = registry.storage<ecs::Plane>();
    auto& boxStorage = registry.storage<ecs::Box>();
    auto& meshStorage = registry.storage<ecs::MeshRef>();
    auto& cameraStorage = registry.storage<ecs::CameraObject>();
    auto& transformStorage = registry.storage<ecs::Transform>();

    for (size_t i = 0; i < entities.size(); i++) {
        const ecs::Entity& e = entities[i];

        if (!transformStorage.has(e)) continue;

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
        } else if (includeCameras && cameraStorage.has(e)) {
            if (cameraStorage.get(e).isPreview) continue;
            constexpr float cameraSelectRadius = 0.6f;
            t = raySphereIntersection(ray, transform.position, cameraSelectRadius);
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

const ecs::Entity* Scene::getSelectedEntity() const {
    if (selectedEntity < 0 || static_cast<size_t>(selectedEntity) >= entities.size())
        return nullptr;
    return &entities[static_cast<size_t>(selectedEntity)];
}

bool Scene::isPreviewingCamera() {
    if (ctx->renderState->renderMode != RenderMode::Preview) return false; // can't preview in render mode (a CameraObject is used but is should not be considered as a preview camera)

    auto& cameras = registry.storage<ecs::CameraObject>();
    for (const auto& e : cameras.entities()) {
        if (cameras.get(e).isPreview)
            return true;
    }
    return false;
}
