#include "inspector_panel.hpp"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

#include "imgui/imgui.h"

#include "core/core.hpp"
#include "core/ecs/components/compositing.hpp"
#include "core/scene/scene.hpp"
#include "editor/ecs/component_ui_registry.hpp"
#include "editor/editor.hpp"
#include "editor/ui_utils.hpp"

void InspectorPanel::draw() {
    const std::optional<ecs::Entity> selectedEntity = Editor::getSelectedEntity();
    bool openNewComponentPopup = false;

    ui::drawWindow(getTitle(), ImGuiWindowFlags_None, [&] {
        if (!selectedEntity.has_value()) {
            ImGui::TextDisabled("No entity selected");
            return;
        }

        ecs::Entity entity = *selectedEntity;
        Scene& scene = Core::getScene();

        ImGui::Text("Add Component");
        ImGui::SameLine();
        if (ui::plusButton("AddComponent")) openNewComponentPopup = true;

        ecs::ComponentUiRegistry::get().draw(scene.getRegistry(), entity);
    });

    if (!selectedEntity.has_value()) return;

    if (openNewComponentPopup) ImGui::OpenPopup("Add Component");
    drawAddComponentPopup(Core::getScene(), *selectedEntity);
}

void InspectorPanel::drawAddComponentPopup(Scene& scene, ecs::Entity entity) {
    if (!ui::beginCenteredModal("Add Component")) return;

    std::vector<const ecs::ComponentType*> sortedTypes;
    for (const ecs::ComponentType& ct : ecs::ComponentType::all()) sortedTypes.push_back(&ct);
    std::stable_sort(
        sortedTypes.begin(), sortedTypes.end(),
        [](const ecs::ComponentType* a, const ecs::ComponentType* b) { return a->getGroup() < b->getGroup(); });

    std::unordered_map<std::string, const ecs::ComponentType*> typeById;
    for (const ecs::ComponentType* ct : sortedTypes) typeById[ct->getId()] = ct;

    auto& registry = scene.getRegistry();

    ImGui::BeginChild("AddComponentList", ImVec2(ui::kButtonSize.x + 40.0f, 450.0f), ImGuiChildFlags_Borders);

    std::string currentGroup;
    bool groupOpen = false;
    for (const ecs::ComponentType* type : sortedTypes) {
        if (type->getGroup() == "internal") continue;

        if (type->getGroup() != currentGroup) {
            currentGroup = type->getGroup();
            std::string groupLabel = currentGroup;
            if (!groupLabel.empty()) groupLabel[0] = static_cast<char>(std::toupper(groupLabel[0]));
            ImGui::Spacing();
            groupOpen = ImGui::CollapsingHeader(groupLabel.c_str());
        }
        if (!groupOpen) continue;

        const bool alreadyPresent = registry.has(entity, *type);

        const bool disabled = alreadyPresent || !registry.canAdd(entity, *type);
        if (disabled) ImGui::BeginDisabled();

        const std::string label = type->getIcon() + " " + type->getLabel();
        if (ImGui::Button(label.c_str(), {-FLT_MIN, 0})) {
            registry.add(entity, *type);
            ImGui::CloseCurrentPopup();
        }

        if (disabled) ImGui::EndDisabled();

        const bool hasTooltip =
            alreadyPresent || !type->getDescription().empty() || !type->getSlot().empty() || !type->getNeeds().empty();

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && hasTooltip) {
            ImGui::BeginTooltip();
            if (alreadyPresent) ImGui::TextDisabled("Already added");
            if (!type->getDescription().empty()) ImGui::TextUnformatted(type->getDescription().c_str());
            if (!type->getSlot().empty())
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "One %s per entity", type->getSlot().c_str());
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

    ImGui::EndChild();

    ui::endCenteredModal();
}
