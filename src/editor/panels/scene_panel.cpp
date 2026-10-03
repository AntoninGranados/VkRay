#include "scene_panel.hpp"

#include <format>
#include <functional>

#include "FontAwesome/IconsFontAwesome7.h"
#include "imgui/imgui.h"

#include "core/core.hpp"
#include "core/ecs/components/core.hpp"
#include "core/scene/scene.hpp"
#include "core/scene/scene_serializer.hpp"
#include "editor/editor.hpp"
#include "editor/ui_utils.hpp"
#include "utils/log.hpp"
#include "utils/string_utils.hpp"

void ScenePanel::draw() {
    Scene& scene = Core::getScene();
    ecs::Registry& reg = scene.getRegistry();

    ui::setNextWindowFixed();
    ui::drawWindow(getTitle(), ImGuiWindowFlags_AlwaysAutoResize, [&] {
        if (ImGui::Button(ICON_FA_UPLOAD " Load Scene", {-FLT_MIN, 0})) {
            if (auto path = ui::openFileDialog({{"Scene", "json"}}, "assets/scenes/")) {
                if (SceneSerializer::load(scene, path->string())) {
                    const ecs::Entity camera = scene.getCamera();
                    Editor::selectEntity(camera == scene.getDefaultCamera() ? std::nullopt : std::optional{camera});
                    Log::success("ScenePanel", std::format("Scene loaded: {}", path->string()));
                }
            }
        }

        if (ImGui::Button(ICON_FA_FLOPPY_DISK " Save Scene", {-FLT_MIN, 0})) {
            if (auto path = ui::saveFileDialog({{"Scene", "json"}}, "assets/scenes/", "untitled.json", "json")) {
                if (SceneSerializer::save(scene, path->string())) {
                    Log::success("ScenePanel", std::format("Scene saved: {}", path->string()));
                }
            }
        }

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
                if (depth <= 1) flags |= ImGuiTreeNodeFlags_DefaultOpen;
                if (Editor::getSelectedEntity() == entity) flags |= ImGuiTreeNodeFlags_Selected;

                const bool open = ImGui::TreeNodeEx((void*)(uintptr_t)entity.getId(), flags, "%s", name.c_str());

                if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) Editor::selectEntity(entity);

                if (open) {
                    for (const ecs::Entity& child : children) drawNode(child, depth + 1);
                    ImGui::TreePop();
                }
            };

            drawNode(scene.getSceneRoot(), 0);
            drawNode(scene.getMaterialsRoot(), 0);
            drawNode(scene.getAssetsRoot(), 0);
            drawNode(scene.getObjectsRoot(), 0);
        }
        ImGui::EndChild();

        if (ui::plusButton("AddEntity")) ImGui::OpenPopup("AddEntityPopup");
        if (ImGui::BeginPopup("AddEntityPopup")) {
            if (ImGui::Selectable("Object")) {
                const ecs::Entity e =
                    scene.createNamedEntity(std::format("Entity-uid[{:02d}]", rand()), scene.getObjectsRoot());
                reg.add(e, ecs::Transform);
                Editor::selectEntity(e);
            }
            if (ImGui::Selectable("Material")) {
                const ecs::Entity e =
                    scene.createNamedEntity(std::format("Material-uid[{:02d}]", rand()), scene.getMaterialsRoot());
                reg.add(e, ecs::Material);
                Editor::selectEntity(e);
            }
            if (ImGui::Selectable("Mesh Asset")) {
                if (auto path = ui::openFileDialog({{"OBJ Mesh", "obj"}}, "assets/models/")) {
                    const ecs::Entity meshAssetEntity =
                        scene.loadMeshAsset(snakeCaseToLabel(path->stem().string()), path->string());
                    if (meshAssetEntity != ecs::Entity{}) {
                        Editor::selectEntity(meshAssetEntity);
                        Log::success("ScenePanel", std::format("Loaded mesh: {}", path->string()));
                    }
                }
            }
            if (ImGui::Selectable("Empty")) {
                const ecs::Entity e =
                    scene.createNamedEntity(std::format("Entity-uid[{:02d}]", rand()), scene.getObjectsRoot());
                Editor::selectEntity(e);
            }
            ImGui::EndPopup();
        }
        ImGui::SameLine();

        const std::optional<ecs::Entity> selectedEntity = Editor::getSelectedEntity();
        const bool canDelete = selectedEntity.has_value() && !reg.has(*selectedEntity, ecs::Locked);
        if (!canDelete) ImGui::BeginDisabled();
        if (ui::minusButton("DeleteEntity")) {
            ecs::Entity entityToDelete = *selectedEntity;
            scene.getAnimationStore().remove(reg, entityToDelete);
            const auto parent = reg.getParent(entityToDelete);
            if (parent.has_value() && *parent == scene.getMaterialsRoot()) {
                auto& materialRefStorage = reg.storage(ecs::MaterialRef);
                for (size_t i = 0; i < materialRefStorage.size(); i++) {
                    ecs::Component& ref = materialRefStorage.get(materialRefStorage.entities()[i]);
                    if (ref.get<ecs::Entity>("handle") == entityToDelete)
                        ref.set<ecs::Entity>("handle", scene.getDefaultMaterial());
                }
                reg.markChanged(ecs::MaterialRef);
            } else if (parent.has_value() && *parent == scene.getAssetsRoot()) {
                auto& meshRefStorage = reg.storage(ecs::MeshRef);
                for (size_t i = 0; i < meshRefStorage.size(); i++) {
                    ecs::Component& ref = meshRefStorage.get(meshRefStorage.entities()[i]);
                    if (ref.get<ecs::Entity>("handle") == entityToDelete)
                        ref.set<ecs::Entity>("handle", scene.getDefaultMesh());
                }
                reg.markChanged(ecs::MeshRef);
            }
            reg.destroyEntity(entityToDelete);
            Editor::selectEntity(std::nullopt);
        }
        if (!canDelete) ImGui::EndDisabled();
    });
}
