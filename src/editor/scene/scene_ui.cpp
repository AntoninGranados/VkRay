#include "scene_ui.hpp"

#include <algorithm>
#include <format>
#include <vector>

#include "FontAwesome/IconsFontAwesome7.h"
#include "imgui/imgui.h"

#include "utils/log.hpp"
#include "core/core.hpp"
#include "core/scene/scene.hpp"
#include "editor/ecs/component_ui_registry.hpp"
#include "editor/ui_constants.hpp"
#include "material_ui.hpp"

void SceneUI::drawInspectors(Scene& scene, SceneSelection& selection) {
    drawSelectedEntityUI(scene, selection);
    drawSelectedMaterialUI(scene, selection);
    drawSelectedMeshAssetUI(scene, selection);
}

void SceneUI::drawSelectedEntityUI(Scene& scene, SceneSelection& selection) {
    if (selection.entity < 0) return;
    ecs::Entity& e = scene.getEntities()[static_cast<size_t>(selection.entity)];
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
        if (uiReg.draw(scene.getRegistry(), e)) scene.update();
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
                    if (!funcsMap.at(requirement).has(scene.getRegistry(), e)) {
                        verifyRestrictions = false;
                        Log::warn("SceneEditor", std::format("Missing component {}", componentLabel(requirement)));
                    }
                }
                for (auto& conflict : restrictions.conflicts) {
                    if (funcsMap.at(conflict).has(scene.getRegistry(), e)) {
                        verifyRestrictions = false;
                        Log::warn("SceneEditor", std::format("Conflicting component {}", componentLabel(conflict)));
                    }
                }

                if (verifyRestrictions) {
                    funcs.add(scene.getRegistry(), e);
                    Core::requestAccumulationRestart();
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

    if (!open) selection.entity = -1;
}

void SceneUI::drawSelectedMaterialUI(Scene& scene, SceneSelection& selection) {
    if (selection.material < 0) return;

    bool open = true;
    ImGui::SetNextWindowBgAlpha(ui::kWindowBgAlpha);
    ImGui::SetNextWindowSizeConstraints({250.0f, 0.0f}, {250.0f, 600.0f});
    ImGui::Begin("Material", &open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing);
    {
        auto& mat = scene.getMaterials()[static_cast<size_t>(selection.material)];
        if (drawMaterialUI(mat)) scene.update();
    }
    ImGui::End();

    if (!open) selection.material = -1;
}

void SceneUI::drawSelectedMeshAssetUI(Scene& scene, SceneSelection& selection) {
    if (selection.meshAsset < 0) return;

    bool open = true;
    ImGui::SetNextWindowBgAlpha(ui::kWindowBgAlpha);
    ImGui::SetNextWindowSizeConstraints({250.0f, 0.0f}, {250.0f, 600.0f});
    ImGui::Begin("Mesh Asset", &open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing);
    {
        if (drawMeshAssetUI(scene.getMeshAssets()[static_cast<size_t>(selection.meshAsset)])) scene.update();
    }
    ImGui::End();

    if (!open) selection.meshAsset = -1;
}
