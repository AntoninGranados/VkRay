#include "scene_ui.hpp"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

#include "FontAwesome/IconsFontAwesome7.h"
#include "imgui/imgui.h"

#include "core/core.hpp"
#include "core/scene/scene.hpp"
#include "editor/ecs/component_ui_registry.hpp"
#include "editor/ui_utils.hpp"
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
        std::vector<const ecs::ComponentType*> sortedTypes;
        for (const ecs::ComponentType& ct : ecs::ComponentType::all()) sortedTypes.push_back(&ct);
        std::sort(sortedTypes.begin(), sortedTypes.end(),
            [](const ecs::ComponentType* a, const ecs::ComponentType* b) {
                return a->getGroup() < b->getGroup();
            }
        );

        std::unordered_map<std::string, const ecs::ComponentType*> typeById;
        for (const ecs::ComponentType* ct : sortedTypes) typeById[ct->getId()] = ct;

        auto& registry = scene.getRegistry();

        std::string currentGroup;
        for (const ecs::ComponentType* type : sortedTypes) {
            if (type->getGroup() != currentGroup) {
                currentGroup = type->getGroup();
                std::string groupLabel = currentGroup;
                if (!groupLabel.empty()) groupLabel[0] = static_cast<char>(std::toupper(groupLabel[0]));
                ImGui::Spacing();
                ImGui::SeparatorText(groupLabel.c_str());
            }

            const bool alreadyPresent = registry.has(e, *type);

            std::vector<std::string> conflicting;
            for (const auto& cid : type->getConflicts()) {
                auto it = typeById.find(cid);
                if (it != typeById.end() && registry.has(e, *it->second))
                    conflicting.push_back(it->second->getLabel());
            }

            std::vector<std::string> missing;
            for (const auto& nid : type->getNeeds()) {
                auto it = typeById.find(nid);
                if (it == typeById.end() || !registry.has(e, *it->second))
                    missing.push_back(it != typeById.end() ? it->second->getLabel() : nid);
            }

            const bool disabled = alreadyPresent || !conflicting.empty() || !missing.empty();
            if (disabled) ImGui::BeginDisabled();

            const std::string label = type->getIcon() + " " + type->getLabel();
            if (ImGui::Button(label.c_str(), ui::kButtonSize)) {
                registry.add(e, *type);
                Core::requestAccumulationRestart();
                ImGui::CloseCurrentPopup();
            }

            if (disabled) ImGui::EndDisabled();

            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && disabled) {
                ImGui::BeginTooltip();
                if (alreadyPresent) {
                    ImGui::Text("Already added");
                } else if (!conflicting.empty()) {
                    ImGui::Text("Conflicts with:");
                    for (const auto& name : conflicting) ImGui::BulletText("%s", name.c_str());
                } else {
                    ImGui::Text("Requires:");
                    for (const auto& name : missing) ImGui::BulletText("%s", name.c_str());
                }
                ImGui::EndTooltip();
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
    ImGui::SetNextWindowSizeConstraints({250.0f, 0.0f}, {250.0f, 600.0f});
    ImGui::Begin("Material", &open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing);
    {
        if (drawMaterialUI(selection.material)) scene.update();
    }
    ImGui::End();

    if (!open) selection.material = -1;
}

void SceneUI::drawSelectedMeshAssetUI(Scene& scene, SceneSelection& selection) {
    if (selection.meshAsset < 0) return;

    bool open = true;
    ImGui::SetNextWindowSizeConstraints({250.0f, 0.0f}, {250.0f, 600.0f});
    ImGui::Begin("Mesh Asset", &open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing);
    {
        if (drawMeshAssetUI(scene.getMeshAssets()[static_cast<size_t>(selection.meshAsset)])) scene.update();
    }
    ImGui::End();

    if (!open) selection.meshAsset = -1;
}
