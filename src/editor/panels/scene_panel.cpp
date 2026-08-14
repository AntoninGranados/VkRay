#include "scene_panel.hpp"

#include <algorithm>
#include <format>

#include <nfd.hpp>
#include "imgui/imgui.h"
#include "FontAwesome/IconsFontAwesome7.h"

#include "utils/log.hpp"
#include "core/core.hpp"
#include "core/ecs/components.hpp"
#include "core/scene/scene.hpp"
#include "core/scene/scene_serializer.hpp"
#include "editor/parameter_ui.hpp"
#include "editor/ui_utils.hpp"


void ScenePanel::content() {
    Scene& scene = Core::getScene();

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
                    scene.getCamera().clearPreviewCamera();
                    selection.entity.reset();
                    for (const ecs::Entity& e : scene.getEntities()) {
                        if (scene.getRegistry().has(e, ecs::Camera)) {
                            selection.entity = e;
                            scene.getCamera().setPreviewCamera(e);
                            break;
                        }
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
        bool openNewMeshAssetPopup = false;

        if (ImGui::BeginTable("Entities", 2, ImGuiTableFlags_None)) {
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Entities");
            if (ImGui::BeginListBox("##Entities", ImVec2(-FLT_MIN, 0.0f))) {
                ecs::Registry& reg = scene.getRegistry();
                for (size_t i = 0; i < scene.getEntities().size(); i++) {
                    const ecs::Entity& e = scene.getEntities()[i];
                    if (reg.has(e, ecs::Material)) continue;

                    std::string displayName = "???";
                    if (reg.has(e, ecs::Name))
                        displayName = reg.get(e, ecs::Name).get<std::string>("value");
                    if (displayName.empty()) displayName = "???";

                    if (ImGui::Selectable(displayName.c_str(), selection.entity == e)) {
                        if (selection.entity != e) {
                            scene.getCamera().clearPreviewCamera();
                            selection.entity = e;
                            Core::requestAccumulationRestart();
                        }
                    }
                }
                ImGui::EndListBox();
            }

            ImGui::TableSetColumnIndex(1);
            ImGui::NewLine();
            if (ImGui::Button("+##Entity", ImVec2(32, 0))) openNewObjectPopup = true;
            if (ImGui::Button("-##Entity", ImVec2(32, 0)) && selection.entity.has_value()) {
                const ecs::Entity e = *selection.entity;
                scene.getAnimationStore().remove(e);
                scene.getRegistry().destroyEntity(e);
                auto& ents = scene.getEntities();
                ents.erase(std::find(ents.begin(), ents.end(), e));
                scene.update();
                selection.entity.reset();
            }

            ImGui::EndTable();
        }

        if (ImGui::BeginTable("Materials", 2, ImGuiTableFlags_None)) {
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Materials");
            if (ImGui::BeginListBox("##Materials", ImVec2(-FLT_MIN, 0.0f))) {
                ecs::Registry& reg = scene.getRegistry();
                const auto& matEnts = reg.storage(ecs::Material).entities();
                for (size_t i = 0; i < matEnts.size(); i++) {
                    const ecs::Entity matEntity = matEnts[i];
                    std::string displayName = "Material";
                    if (reg.has(matEntity, ecs::Name)) {
                        const std::string& n = reg.get(matEntity, ecs::Name).get<std::string>("value");
                        if (!n.empty()) displayName = n;
                    }
                    std::string label = displayName + "##Material" + std::to_string(i);
                    if (ImGui::Selectable(label.c_str(), selection.entity == matEntity, ImGuiSelectableFlags_AllowDoubleClick)) {
                        if (ImGui::IsMouseDoubleClicked(0))
                            selection.entity = matEntity;
                    }
                }
                ImGui::EndListBox();
            }

            ImGui::TableSetColumnIndex(1);
            ImGui::NewLine();
            if (ImGui::Button("+##Materials", ImVec2(32, 0))) {
                scene.pushMaterial(ecs::Diffuse, std::format("Material-uid[{:02d}]", rand()));
                scene.update();
            }
            if (ImGui::Button("-##Materials", ImVec2(32, 0))) {
                ecs::Registry& reg = scene.getRegistry();
                const auto& matEnts = reg.storage(ecs::Material).entities();
                int selMatIdx = -1;
                if (selection.entity.has_value()) {
                    for (int i = 0; i < static_cast<int>(matEnts.size()); i++) {
                        if (matEnts[i] == *selection.entity) { selMatIdx = i; break; }
                    }
                }
                if (selMatIdx > 0) {
                    const ecs::Entity matEntity = matEnts[selMatIdx];
                    const int lastSlot = static_cast<int>(matEnts.size()) - 1;
                    scene.getAnimationStore().remove(matEntity);

                    auto& entities = scene.getEntities();
                    entities.erase(std::find(entities.begin(), entities.end(), matEntity));

                    auto& matRefs = reg.storage(ecs::MaterialRef);
                    for (size_t i = 0; i < matRefs.size(); i++) {
                        ecs::Component& ref = matRefs.get(matRefs.entities()[i]);
                        const int h = ref.get<int>("handle");
                        if (h == selMatIdx)
                            ref.set<int>("handle", 0);
                        else if (h == lastSlot)
                            ref.set<int>("handle", selMatIdx);
                    }
                    reg.destroyEntity(matEntity);
                    scene.update();
                    selection.entity.reset();
                }
            }

            ImGui::EndTable();
        }

        if (ImGui::BeginTable("MeshAssets", 2, ImGuiTableFlags_None)) {
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Mesh Assets");
            if (ImGui::BeginListBox("##MeshAssets", ImVec2(-FLT_MIN, 0.0f))) {
                for (size_t i = 0; i < scene.getMeshAssets().size(); i++) {
                    const std::string& meshName = scene.getMeshAssets()[i].getName();
                    const char* display = meshName.empty() ? "Mesh" : meshName.c_str();
                    std::string label = std::string(display) + "##MeshAsset" + std::to_string(i);
                    if (ImGui::Selectable(label.c_str(), selection.meshAsset == static_cast<int>(i), ImGuiSelectableFlags_AllowDoubleClick)) {
                        if (ImGui::IsMouseDoubleClicked(0)) selection.meshAsset = static_cast<int>(i);
                    }
                }
                ImGui::EndListBox();
            }

            ImGui::TableSetColumnIndex(1);
            ImGui::NewLine();
            if (ImGui::Button("+##MeshAssets", ImVec2(32, 0))) openNewMeshAssetPopup = true;
            if (ImGui::Button("-##MeshAssets", ImVec2(32, 0)) &&
                selection.meshAsset > 0 &&
                selection.meshAsset < static_cast<int>(scene.getMeshAssets().size())) {
                const int removed = selection.meshAsset;
                scene.getMeshAssets().erase(scene.getMeshAssets().begin() + removed);

                auto& meshes = scene.getRegistry().storage(ecs::MeshRef);
                const auto& refEntities = meshes.entities();
                for (size_t i = 0; i < meshes.size(); i++) {
                    ecs::Component& ref = meshes.get(refEntities[i]);
                    const int h = ref.get<int>("handle");
                    if (h == removed)
                        ref.set<int>("handle", 0);
                    else if (h > removed)
                        ref.set<int>("handle", h - 1);
                }
                scene.update();
                selection.meshAsset = -1;
            }

            ImGui::EndTable();
        }

        if (openNewMeshAssetPopup) {
            NFD::Guard guard;
            NFD::UniquePath outPath;
            nfdfilteritem_t filter[1] = { { "OBJ Mesh", "obj" } };
            if (NFD::OpenDialog(outPath, filter, 1, "assets/models/") == NFD_OKAY) {
                const std::string meshPath = outPath.get();
                MeshAsset asset(MeshAsset::nameFromPath(meshPath));
                if (asset.loadFromObj(meshPath)) {
                    Log::success("SceneEditor", std::format("Loaded mesh: {}", asset.getName()));
                    scene.getMeshAssets().push_back(std::move(asset));
                    scene.update();
                }
            }
        }

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
        scene.getEntities().push_back(scene.getRegistry().createEntity());
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
        scene.pushMesh(name, 0, glm::mat3(1.0));
        scene.update();
        ImGui::CloseCurrentPopup();
    }
    if (ImGui::Button(ICON_FA_VIDEO " Camera", ui::kButtonSize)) {
        scene.pushCamera(name, glm::mat3(1.0));
        ImGui::CloseCurrentPopup();
    }
    ui::PushCancelStyleColor();
    if (ImGui::Button(ICON_FA_BAN " Cancel", ui::kButtonSize)) {
        ImGui::CloseCurrentPopup();
    }
    ui::PopCancelStyleColor();

    ImGui::EndPopup();
}
