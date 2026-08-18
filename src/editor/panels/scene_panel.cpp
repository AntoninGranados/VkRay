#include "scene_panel.hpp"

#include <filesystem>
#include <format>
#include <functional>

#include <nfd.hpp>
#include "imgui/imgui.h"
#include "FontAwesome/IconsFontAwesome7.h"

#include "utils/log.hpp"
#include "core/core.hpp"
#include "core/scene/scene.hpp"
#include "core/scene/scene_serializer.hpp"
#include "editor/editor.hpp"
#include "editor/parameter_ui.hpp"
#include "editor/ui_utils.hpp"


void ScenePanel::content() {
    Scene& scene = Core::getScene();
    ecs::Registry& reg = scene.getRegistry();

    ui::setNextWindowFixed();
    ImGui::Begin(ICON_FA_CUBES " Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    {
        if (ImGui::Button(ICON_FA_UPLOAD " Load Scene", { -FLT_MIN, 0 })) {
            NFD::Guard guard;
            NFD::UniquePath outPath;
            nfdfilteritem_t filter[1] = { { "Scene", "json" } };
            if (NFD::OpenDialog(outPath, filter, 1, "assets/scenes/") == NFD_OKAY) {
                LightMode mode = Core::getParameters().get<LightMode>("scene/light_mode");
                if (SceneSerializer::load(scene, mode, outPath.get())) {
                    Core::getParameters().set("scene/light_mode", mode);
                    Editor::clearSelectedEntity();
                    const auto& cameras = reg.storage(ecs::Camera).entities();
                    if (!cameras.empty()) {
                        Editor::setSelectedEntity(cameras[0]);
                        scene.getCamera().setPreviewCamera(cameras[0]);
                    }
                    Core::requestAccumulationRestart();
                    Log::success("ScenePanel", std::format("Scene loaded: {}", outPath.get()));
                }
            }
        }

        if (ImGui::Button(ICON_FA_FLOPPY_DISK " Save Scene", { -FLT_MIN, 0 })) {
            NFD::Guard guard;
            NFD::UniquePath outPath;
            nfdfilteritem_t filter[1] = { { "Scene", "json" } };
            if (NFD::SaveDialog(outPath, filter, 1, "assets/scenes/", "untitled.json") == NFD_OKAY) {
                std::string path = outPath.get();
                if (path.size() < 5 || path.substr(path.size() - 5) != ".json")
                    path += ".json";
                LightMode mode = Core::getParameters().get<LightMode>("scene/light_mode");
                if (SceneSerializer::save(scene, mode, path)) {
                    Log::success("ScenePanel", std::format("Scene saved: {}", path));
                }
            }
        }

        ParameterUI::drawGroup("scene");

        bool openNewObjectPopup = false;

        if (ImGui::BeginChild("##SceneTree", ImVec2(-FLT_MIN, 400.0f), ImGuiChildFlags_Borders)) {
            std::function<void(ecs::Entity, int)> drawNode;
            drawNode = [&](ecs::Entity entity, int depth) {
                const auto& children = reg.getChildren(entity);

                std::string name = "???";
                if (reg.has(entity, ecs::Name)) {
                    const std::string& n = reg.get(entity, ecs::Name).get<std::string>("value");
                    if (!n.empty()) name = n;
                }

                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
                if (children.empty()) flags |= ImGuiTreeNodeFlags_Leaf;
                if (depth == 0) flags |= ImGuiTreeNodeFlags_DefaultOpen;
                if (Editor::getSelectedEntity() == entity) flags |= ImGuiTreeNodeFlags_Selected;

                const bool open = ImGui::TreeNodeEx((void*)(uintptr_t)entity.getId(), flags, "%s", name.c_str());

                if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                    scene.getCamera().clearPreviewCamera();
                    Editor::setSelectedEntity(entity);
                    if (reg.has(entity, ecs::Camera))
                        scene.getCamera().setPreviewCamera(entity);
                    Core::requestAccumulationRestart();
                }

                if (open) {
                    for (const ecs::Entity& child : children)
                        drawNode(child, depth + 1);
                    ImGui::TreePop();
                }
            };

            drawNode(scene.getMaterialsRoot(), 0);
            drawNode(scene.getAssetsRoot(), 0);
            drawNode(scene.getObjectsRoot(), 0);
        }
        ImGui::EndChild();

        if (ImGui::Button("+ Object")) openNewObjectPopup = true;
        ImGui::SameLine();
        if (ImGui::Button("+ Material")) {
            scene.pushMaterial(ecs::Diffuse, std::format("Material-uid[{:02d}]", rand()));
            scene.update();
        }
        ImGui::SameLine();
        if (ImGui::Button("+ Mesh")) {
            NFD::Guard guard;
            NFD::UniquePath outPath;
            nfdfilteritem_t filter[1] = { { "OBJ Mesh", "obj" } };
            if (NFD::OpenDialog(outPath, filter, 1, "assets/models/") == NFD_OKAY) {
                const std::string meshPath = outPath.get();
                const ecs::Entity meshAssetEntity = scene.pushMeshAsset(
                    std::filesystem::path(meshPath).stem().string(), meshPath);
                if (meshAssetEntity != ecs::Entity{}) {
                    scene.update();
                    Editor::setSelectedEntity(meshAssetEntity);
                    Log::success("ScenePanel", std::format("Loaded mesh: {}", meshPath));
                }
            }
        }
        ImGui::SameLine();

        const std::optional<ecs::Entity> selectedEntity = Editor::getSelectedEntity();
        const bool canDelete = selectedEntity.has_value()
            && *selectedEntity != scene.getDefaultMaterial()
            && *selectedEntity != scene.getDefaultMeshAsset()
            && *selectedEntity != scene.getMaterialsRoot()
            && *selectedEntity != scene.getAssetsRoot()
            && *selectedEntity != scene.getObjectsRoot();
        if (!canDelete) ImGui::BeginDisabled();
        if (ImGui::Button("- Delete")) {
            const ecs::Entity entityToDelete = *selectedEntity;
            scene.getAnimationStore().remove(entityToDelete);
            const auto parent = reg.getParent(entityToDelete);
            if (parent.has_value() && *parent == scene.getMaterialsRoot()) {
                auto& materialRefStorage = reg.storage(ecs::MaterialRef);
                for (size_t i = 0; i < materialRefStorage.size(); i++) {
                    ecs::Component& ref = materialRefStorage.get(materialRefStorage.entities()[i]);
                    if (ref.get<ecs::Entity>("handle") == entityToDelete)
                        ref.set<ecs::Entity>("handle", scene.getDefaultMaterial());
                }
            } else if (parent.has_value() && *parent == scene.getAssetsRoot()) {
                auto& meshRefStorage = reg.storage(ecs::MeshRef);
                for (size_t i = 0; i < meshRefStorage.size(); i++) {
                    ecs::Component& ref = meshRefStorage.get(meshRefStorage.entities()[i]);
                    if (ref.get<ecs::Entity>("handle") == entityToDelete)
                        ref.set<ecs::Entity>("handle", scene.getDefaultMeshAsset());
                }
            }
            reg.destroyEntity(entityToDelete);
            scene.update();
            Editor::clearSelectedEntity();
        }
        if (!canDelete) ImGui::EndDisabled();

        if (openNewObjectPopup) ImGui::OpenPopup("New Object");
        drawNewObjectPopUp(scene);
    }
    ImGui::End();
}

void ScenePanel::drawNewObjectPopUp(Scene& scene) {
    ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(mainViewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (!ImGui::BeginPopupModal("New Object", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) return;

    std::string name = std::format("Entity-uid[{:02d}]", rand());

    if (ImGui::Button(ICON_FA_BORDER_NONE " Empty", ui::kButtonSize)) {
        ecs::Entity e = scene.getRegistry().createEntity(scene.getObjectsRoot());
        scene.getRegistry().add(e, ecs::Name);
        scene.getRegistry().get(e, ecs::Name).set<std::string>("value", name);
        scene.update();
        ImGui::CloseCurrentPopup();
    }
    if (ImGui::Button(ICON_FA_CIRCLE " Sphere", ui::kButtonSize)) {
        scene.pushSphere(name, glm::vec3(0.0, 0.0, 0.0), 1.0);
        scene.update();
        ImGui::CloseCurrentPopup();
    }
    if (ImGui::Button(ICON_FA_SQUARE " Plane", ui::kButtonSize)) {
        scene.pushPlane(name, glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
        scene.update();
        ImGui::CloseCurrentPopup();
    }
    if (ImGui::Button(ICON_FA_BOX " Box", ui::kButtonSize)) {
        scene.pushBox(name, glm::vec3(-1.0, -1.0, -1.0), glm::vec3(1.0, 1.0, 1.0));
        scene.update();
        ImGui::CloseCurrentPopup();
    }
    if (ImGui::Button(ICON_FA_SQUARE " Quad", ui::kButtonSize)) {
        scene.pushQuad(name, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), glm::vec2(1, 1));
        scene.update();
        ImGui::CloseCurrentPopup();
    }
    if (ImGui::Button(ICON_FA_CUBE " Mesh", ui::kButtonSize)) {
        scene.pushMesh(name, scene.getDefaultMeshAsset(), glm::mat4(1.0));
        scene.update();
        ImGui::CloseCurrentPopup();
    }
    if (ImGui::Button(ICON_FA_VIDEO " Camera", ui::kButtonSize)) {
        scene.pushCamera(name, glm::mat4(1.0));
        scene.update();
        ImGui::CloseCurrentPopup();
    }
    ui::PushCancelStyleColor();
    if (ImGui::Button(ICON_FA_BAN " Cancel", ui::kButtonSize)) {
        ImGui::CloseCurrentPopup();
    }
    ui::PopCancelStyleColor();

    ImGui::EndPopup();
}
