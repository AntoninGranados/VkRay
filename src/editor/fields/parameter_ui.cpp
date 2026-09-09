#include "parameter_ui.hpp"

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/fields/parameters.hpp"
#include "imgui/imgui.h"

#include "core/core.hpp"
#include "editor/fields/field_ui.hpp"
#include "editor/ui_utils.hpp"

namespace ParameterUI {

namespace {

bool drawParameter(Parameter& p) {
    ImGui::PushID(p.getId().c_str());
    ImGui::BeginGroup();

    bool changed = ui::drawField(p, "##value");

    ImGui::EndGroup();
    if (p.getDescription() && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        ImGui::SetTooltip("%s", p.getDescription()->c_str());
    ImGui::PopID();

    if (changed) p.sync();
    return changed;
}

std::string groupLabel(const FieldPath& id) {
    const auto& labels = Core::getParameters().getNodeLabels();
    auto it = labels.find(id.generic_string());
    return it != labels.end() ? it->second : id.filename().string();
}

void clusterByCondition(std::vector<ui::FieldGroup>& groups) {
    std::vector<ui::FieldGroup> result;
    std::unordered_map<std::string, size_t> conditionGroups;

    for (auto& item : groups) {
        Parameter* param = item.field ? static_cast<Parameter*>(item.field) : nullptr;
        if (!param || !param->getCondition()) {
            result.push_back(std::move(item));
            continue;
        }

        const ParameterCondition& cond = *param->getCondition();
        const std::string key = cond.param.string();
        if (!conditionGroups.contains(key)) {
            conditionGroups[key] = result.size();
            ui::FieldGroup group;
            group.showHeader = false;
            group.disabledWhen = [cond] { return Core::getParameters().get<bool>(cond.param) != cond.when; };
            result.push_back(std::move(group));
        }
        result[conditionGroups.at(key)].children.push_back(std::move(item));
    }

    groups = std::move(result);
    for (auto& item : groups)
        if (item.showHeader) clusterByCondition(item.children);
}

} // namespace

void drawGroup(const FieldPath& root) {
    static std::vector<ui::FieldGroup> cached;

    if (cached.empty()) {
        std::vector<Field*> ptrs;
        for (const auto& param : Core::getParameters().getAll()) ptrs.push_back(param.get());
        cached = ui::buildFieldGroups(ptrs);
        clusterByCondition(cached);
    }

    for (auto& child : cached) {
        if (child.id != root) continue;

        bool restartNeeded = false;
        auto drawLeaf = [&](Field& field, const std::string&) {
            Parameter& param = static_cast<Parameter&>(field);
            const bool changed = drawParameter(param);
            if (changed && param.isRestartingAnimation()) restartNeeded = true;
            return changed;
        };

        ui::drawFieldGroups(child.children, "", drawLeaf, groupLabel);
        if (restartNeeded) Core::markRenderDirty();
        return;
    }
}

} // namespace ParameterUI
