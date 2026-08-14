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

void SceneUI::drawInspectors(Scene& scene, SceneSelection& selection) {
    drawSelectedEntityUI(scene, selection);
    drawSelectedMeshAssetUI(scene, selection);
}

void SceneUI::drawSelectedEntityUI(Scene& scene, SceneSelection& selection) {
    if (!selection.entity.has_value()) return;
    ecs::Entity& e = *selection.entity;
    bool openNewComponentPopup = false;

    bool open = true;
    ImGui::SetNextWindowSizeConstraints({250.0f, 50.0f}, {FLT_MAX, FLT_MAX});
    ImGui::SetNextWindowSize({300.0f, 400.0f}, ImGuiCond_FirstUseEver);
    ImGui::Begin("Entity", &open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing);
    {
        ImGui::Text("Add Component");
        ImGui::SameLine();
        if (ImGui::Button("+##AddComponent", {32, 0})) {
            openNewComponentPopup = true;
        }

        auto& reg = scene.getRegistry();
        auto& uiReg = ecs::ComponentUiRegistry::get();
        bool changed = uiReg.draw(reg, e);
        reg.flush();
        if (changed) {
            Core::requestAccumulationRestart();
            scene.update();
        }
    }
    ImGui::End();

    if (openNewComponentPopup) ImGui::OpenPopup("Add Component");
    ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(mainViewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Add Component", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        std::vector<const ecs::ComponentType*> sortedTypes;
        for (const ecs::ComponentType& ct : ecs::ComponentType::all()) sortedTypes.push_back(&ct);
        std::stable_sort(sortedTypes.begin(), sortedTypes.end(),
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

            const bool disabled = alreadyPresent || !conflicting.empty();
            if (disabled) ImGui::BeginDisabled();

            const std::string label = type->getIcon() + " " + type->getLabel();
            if (ImGui::Button(label.c_str(), ui::kButtonSize)) {
                registry.add(e, *type);
                Core::requestAccumulationRestart();
                scene.update();
                ImGui::CloseCurrentPopup();
            }

            if (disabled) ImGui::EndDisabled();

            const bool hasTooltip = alreadyPresent
                || !type->getDescription().empty()
                || !type->getConflicts().empty()
                || !type->getNeeds().empty();

            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && hasTooltip) {
                ImGui::BeginTooltip();
                if (alreadyPresent)
                    ImGui::TextDisabled("Already added");
                if (!type->getDescription().empty())
                    ImGui::TextUnformatted(type->getDescription().c_str());
                if (!type->getConflicts().empty()) {
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Conflicts:");
                    for (const auto& cid : type->getConflicts()) {
                        auto it = typeById.find(cid);
                        ImGui::BulletText("%s", it != typeById.end() ? it->second->getLabel().c_str() : cid.c_str());
                    }
                }
                if (!type->getNeeds().empty()) {
                    ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.4f, 1.0f), "Needs:");
                    for (const auto& nid : type->getNeeds()) {
                        auto it = typeById.find(nid);
                        ImGui::BulletText("%s", it != typeById.end() ? it->second->getLabel().c_str() : nid.c_str());
                    }
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

    if (!open) selection.entity.reset();
}


void SceneUI::drawSelectedMeshAssetUI(Scene& scene, SceneSelection& selection) {
    if (selection.meshAsset < 0) return;

    bool open = true;
    ImGui::SetNextWindowSizeConstraints({250.0f, 50.0f}, {FLT_MAX, FLT_MAX});
    ImGui::SetNextWindowSize({300.0f, 400.0f}, ImGuiCond_FirstUseEver);
    ImGui::Begin("Mesh Asset", &open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing);
    {
        if (drawMeshAssetUI(scene.getMeshAssets()[static_cast<size_t>(selection.meshAsset)])) scene.update();
    }
    ImGui::End();

    if (!open) selection.meshAsset = -1;
}
