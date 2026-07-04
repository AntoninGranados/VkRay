#include "scene_editor_ui.hpp"


#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

#include "FontAwesome/IconsFontAwesome7.h"

#include "imgui/imgui.h"
#include "imgui/ImGuizmo.h"
#include <nfd.hpp>

#include "app/notification_handler.hpp"
#include "editor/ui_constants.hpp"
#include "./scene.hpp"

void SceneEditorUI::drawGuizmo(Scene& scene, const glm::mat4& view, const glm::mat4& proj) {
    if (scene.selectedEntity < 0) return;

    ecs::Entity e = scene.entities[scene.selectedEntity];
    if (!scene.registry.has<ecs::Transform>(e)) return;

    ecs::Transform& t = scene.registry.get<ecs::Transform>(e);
    glm::mat4 model = t.local;

    int opFlags = ImGuizmo::OPERATION::TRANSLATE | ImGuizmo::OPERATION::ROTATE | ImGuizmo::OPERATION::SCALE;
    // Keep gizmo orientation in world space: avoid mixing scale with other ops.
    if ((opFlags & ImGuizmo::OPERATION::SCALE) && (opFlags & (ImGuizmo::OPERATION::TRANSLATE | ImGuizmo::OPERATION::ROTATE))) {
        opFlags &= ~ImGuizmo::OPERATION::SCALE;
    }

    ImGuizmo::PushID(scene.selectedEntity);
    if (ImGuizmo::Manipulate(
            glm::value_ptr(view),
            glm::value_ptr(proj),
            static_cast<ImGuizmo::OPERATION>(opFlags),
            ImGuizmo::MODE::WORLD,
            glm::value_ptr(model))) {
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
        scene.updated = true;
    }
    ImGuizmo::PopID();
}

void SceneEditorUI::drawUI(Scene& scene) {
    assert(scene.ctx);
    bool openNewObjectPopup = false;
    bool openNewMeshAssetPopup = false;

    if (ImGui::BeginTable("Entities", 2, ImGuiTableFlags_None)) {
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Entities");
        if (ImGui::BeginListBox("##Entities", ImVec2(-FLT_MIN, 0.0f))) {
            for (size_t i = 0; i < scene.entities.size(); i++) {
                const ecs::Entity& e = scene.entities[i];

                std::string displayName = "???";
                if (scene.registry.has<ecs::Name>(e)) displayName = scene.registry.get<ecs::Name>(e).value;
                if (displayName.empty()) displayName = "???";

                if (scene.registry.has<ecs::EditorOnly>(e)) {
                    ImGui::TextDisabled("%s", displayName.c_str());
                } else if (ImGui::Selectable(displayName.c_str(), scene.selectedEntity == i, ImGuiSelectableFlags_AllowDoubleClick)) {
                    if (ImGui::IsMouseDoubleClicked(0)) {
                        scene.selectedEntity = static_cast<int>(i);
                    }
                }
            }

            ImGui::EndListBox();
        }

        ImGui::TableSetColumnIndex(1);
        ImGui::NewLine();
        if (ImGui::Button("+##Entity", ImVec2(32, 0))) openNewObjectPopup = true;
        if (ImGui::Button("-##Entity", ImVec2(32, 0)) && scene.selectedEntity >= 0) {
            ecs::Entity e = scene.entities[static_cast<size_t>(scene.selectedEntity)];
            scene.registry.destroyEntity(e);
            scene.entities.erase(std::next(scene.entities.begin(), scene.selectedEntity));
            scene.updated = true;
            scene.selectedEntity = -1;
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
            for (size_t i = 0; i < scene.materials.size(); i++) {
                const std::string& materialName = scene.materials[i].name;
                const char* display = materialName.empty() ? "Material" : materialName.c_str();
                std::string label = std::string(display) + "##Material" + std::to_string(i);
                if (ImGui::Selectable(label.c_str(), scene.selectedMaterial == static_cast<int>(i), ImGuiSelectableFlags_AllowDoubleClick)) {
                    if (ImGui::IsMouseDoubleClicked(0)) scene.selectedMaterial = static_cast<int>(i);
                }
            }

            ImGui::EndListBox();
        }

        ImGui::TableSetColumnIndex(1);
        ImGui::NewLine();
        if (ImGui::Button("+##Materials", ImVec2(32, 0))) {
            Material mat = DEFAULT_MATERIAL;
            char matName[64];
            std::snprintf(matName, sizeof(matName), "Material-%02d", scene.materialN++);
            mat.name = matName;
            scene.pushMaterial(mat);
            scene.updated = true;
        }
        if (ImGui::Button("-##Materials", ImVec2(32, 0)) &&
            scene.selectedMaterial > 0 &&
            scene.selectedMaterial < static_cast<int>(scene.materials.size())) {
            const int removed = scene.selectedMaterial;
            scene.materials.erase(scene.materials.begin() + removed);

            auto& matRefs = scene.registry.storage<ecs::MaterialRef>();
            const auto& refEntities = matRefs.entities();
            for (size_t i = 0; i < matRefs.size(); i++) {
                ecs::MaterialRef& ref = matRefs.get(refEntities[i]);
                if (ref.handle == removed)
                    ref.handle = 0;
                else if (ref.handle > removed)
                    ref.handle--;
            }
            scene.updated = true;
            scene.selectedMaterial = -1;
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
            for (size_t i = 0; i < scene.meshAssets.size(); i++) {
                const std::string& meshName = scene.meshAssets[i].getName();
                const char* display = meshName.empty() ? "Mesh" : meshName.c_str();
                std::string label = std::string(display) + "##MeshAsset" + std::to_string(i);
                if (ImGui::Selectable(label.c_str(), scene.selectedMeshAsset == static_cast<int>(i), ImGuiSelectableFlags_AllowDoubleClick)) {
                    if (ImGui::IsMouseDoubleClicked(0)) scene.selectedMeshAsset = static_cast<int>(i);
                }
            }
            ImGui::EndListBox();
        }

        ImGui::TableSetColumnIndex(1);
        ImGui::NewLine();
        if (ImGui::Button("+##MeshAssets", ImVec2(32, 0))) openNewMeshAssetPopup = true;
        if (ImGui::Button("-##MeshAssets", ImVec2(32, 0)) &&
            scene.selectedMeshAsset > 0 &&
            scene.selectedMeshAsset < static_cast<int>(scene.meshAssets.size())) {
            const int removed = scene.selectedMeshAsset;
            scene.meshAssets.erase(scene.meshAssets.begin() + removed);

            auto& meshRefs = scene.registry.storage<ecs::MeshRef>();
            const auto& refEntities = meshRefs.entities();
            for (size_t i = 0; i < meshRefs.size(); i++) {
                ecs::MeshRef& ref = meshRefs.get(refEntities[i]);
                if (ref.handle == removed)
                    ref.handle = 0;
                else if (ref.handle > removed)
                    ref.handle--;
            }
            scene.updated = true;
            scene.selectedMeshAsset = -1;
        }

        ImGui::EndTable();
    }

    if (openNewMeshAssetPopup) {
        NFD::Guard guard;
        NFD::UniquePath outPath;
        nfdfilteritem_t filter[1] = { { "OBJ Mesh", "obj" } };
        if (NFD::OpenDialog(outPath, filter, 1, "res/models/") == NFD_OKAY) {
            const std::string meshPath = outPath.get();
            MeshAsset asset(MeshAsset::nameFromPath(meshPath));
            if (asset.loadFromObj(*scene.ctx, meshPath)) {
                scene.meshAssets.push_back(std::move(asset));
                scene.updated = true;
            }
        }
    }

    if (openNewObjectPopup) ImGui::OpenPopup("New Object");
    drawNewObjectPopUp(scene);
}

void SceneEditorUI::drawNewObjectPopUp(Scene& scene) {
    ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(mainViewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (!ImGui::BeginPopupModal("New Object", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) return;

    char nameBuffer[64];
    std::snprintf(nameBuffer, sizeof(nameBuffer), "Entity-%02d", scene.entityN);
    std::string name(nameBuffer);

    if (ImGui::Button(ICON_FA_BORDER_NONE " Empty", ui::kButtonSize)) {
        scene.entities.push_back(scene.registry.createEntity());
        scene.updated = true;
        ImGui::CloseCurrentPopup();
        scene.entityN++;
    }
    if (ImGui::Button(ICON_FA_CIRCLE " Sphere", ui::kButtonSize)) {
        scene.pushSphere(name, glm::vec3(0.0, 0.0, 0.0), 1.0);
        scene.updated = true;
        ImGui::CloseCurrentPopup();
        scene.entityN++;
    }
    if (ImGui::Button(ICON_FA_SQUARE " Plane", ui::kButtonSize)) {
        scene.pushPlane(name, glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
        scene.updated = true;
        ImGui::CloseCurrentPopup();
        scene.entityN++;
    }
    if (ImGui::Button(ICON_FA_BOX " Box", ui::kButtonSize)) {
        scene.pushBox(name, glm::vec3(-1.0, -1.0, -1.0), glm::vec3(1.0, 1.0, 1.0));
        scene.updated = true;
        ImGui::CloseCurrentPopup();
        scene.entityN++;
    }
    if (ImGui::Button(ICON_FA_CUBE " Mesh", ui::kButtonSize)) {
        scene.pushMesh(name, 0, glm::mat3(1.0));
        scene.updated = true;
        ImGui::CloseCurrentPopup();
        scene.entityN++;
    }
    if (ImGui::Button(ICON_FA_VIDEO " Camera", ui::kButtonSize)) {
        scene.pushCamera(name, glm::mat3(1.0));
        ImGui::CloseCurrentPopup();
        scene.entityN++;
    }
    ui::PushCancelStyleColor();
    if (ImGui::Button(ICON_FA_BAN " Cancel", ui::kButtonSize)) {
        ImGui::CloseCurrentPopup();
    }
    ui::PopCancelStyleColor();

    ImGui::EndPopup();
}

void SceneEditorUI::drawSelectedEntityUI(Scene& scene) {
    if (scene.selectedEntity < 0) return;
    ecs::Entity& e = scene.entities[scene.selectedEntity];
    bool openNewComponentPopup = false;

    bool open = true;
    ImGui::SetNextWindowBgAlpha(ui::kWindowBgAlpha);
    ImGui::SetNextWindowSizeConstraints({250.0f, 0.0f}, {250.0f, 600.0f});
    ImGui::Begin(
        "Entity",
        &open,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing);
    {
        ImGui::Text("Add Component");
        ImGui::SameLine();
        if (ImGui::Button("+##AddComponent", {32, 0})) {
            openNewComponentPopup = true;
        }

        auto& uiReg = ecs::ComponentUiRegistry::get();
        scene.updated |= uiReg.draw(*scene.ctx, scene.registry, e);
    }
    ImGui::End();

    if (openNewComponentPopup) ImGui::OpenPopup("Add Component");
    ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(mainViewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Add Component", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        const auto& funcsMap = componentFuncs();
        const auto& restrictionsMap = componentRestrictions();

        std::vector<ComponentId> sortedIds;
        sortedIds.reserve(funcsMap.size());
        for (const auto& [id, _] : funcsMap) {
            sortedIds.push_back(id);
        }

        std::sort(sortedIds.begin(), sortedIds.end(), [](ComponentId a, ComponentId b) {
            const ComponentGroup groupA = componentGroup(a);
            const ComponentGroup groupB = componentGroup(b);
            if (groupA != groupB) {
                return groupA < groupB;
            }
            return componentLabel(a) < componentLabel(b);
        });

        bool firstGroup = true;
        ComponentGroup currentGroup = ComponentGroup::Other;
        for (ComponentId id : sortedIds) {
            const ComponentGroup group = componentGroup(id);
            if (firstGroup || group != currentGroup) {
                if (!firstGroup) {
                    ImGui::Spacing();
                }
                ImGui::SeparatorText(componentGroupLabel(group).c_str());
                currentGroup = group;
                firstGroup = false;
            }

            const auto& funcs = funcsMap.at(id);
            if (ImGui::Button(componentLabel(id).c_str(), ui::kButtonSize)) {
                bool verifyRestrictions = true;
                const auto& restrictions = restrictionsMap.at(id);
                for (auto& requirement : restrictions.requirements) {
                    if (!funcsMap.at(requirement).has(scene.registry, e)) {
                        verifyRestrictions = false;
                        scene.ctx->notifications->pushMessage(NotificationType::Warning, "Missing component " + componentLabel(requirement));
                    }
                }
                for (auto& conflict : restrictions.conflicts) {
                    if (funcsMap.at(conflict).has(scene.registry, e)) {
                        verifyRestrictions = false;
                        scene.ctx->notifications->pushMessage(NotificationType::Warning, "Conflicting component " + componentLabel(conflict));
                    }
                }

                if (!verifyRestrictions) {
                    scene.ctx->notifications->pushMessage(NotificationType::Error, "Failed to add component, not all restrictions met");
                } else {
                    funcs.add(scene.registry, e);
                    *scene.ctx->restartRender = true;
                }
                ImGui::CloseCurrentPopup();
            }
        }

        ui::PushCancelStyleColor();
        if (ImGui::Button(ICON_FA_BAN " Cancel", ui::kButtonSize)) {
            ImGui::CloseCurrentPopup();
        }
        ui::PopCancelStyleColor();
        ImGui::EndPopup();
    }

    if (!open) scene.selectedEntity = -1;
}

void SceneEditorUI::drawSelectedMaterialUI(Scene& scene) {
    if (scene.selectedMaterial < 0) return;

    bool open = true;
    ImGui::SetNextWindowBgAlpha(ui::kWindowBgAlpha);
    ImGui::SetNextWindowSizeConstraints({250.0f, 0.0f}, {250.0f, 600.0f});
    ImGui::Begin("Material", &open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing);
    {
        auto& mat = scene.materials[scene.selectedMaterial];
        scene.updated |= drawMaterialUI(mat);
    }
    ImGui::End();

    if (!open) scene.selectedMaterial = -1;
}

void SceneEditorUI::drawSelectedMeshAssetUI(Scene& scene) {
    if (scene.selectedMeshAsset < 0) return;

    bool open = true;
    ImGui::SetNextWindowBgAlpha(ui::kWindowBgAlpha);
    ImGui::SetNextWindowSizeConstraints({250.0f, 0.0f}, {250.0f, 600.0f});
    ImGui::Begin("Mesh Asset", &open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing);
    {
        bool changed = drawMeshAssetUI(scene.meshAssets[scene.selectedMeshAsset]);
        scene.updated |= changed;
    }
    ImGui::End();

    if (!open) scene.selectedMeshAsset = -1;
}
