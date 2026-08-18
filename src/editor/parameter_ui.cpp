#include "parameter_ui.hpp"

#include <algorithm>
#include <unordered_map>
#include <vector>

#include "imgui/imgui.h"

#include "core/core.hpp"
#include "editor/field_ui.hpp"
#include "editor/ui_utils.hpp"

bool ParameterUI::drawParameter(Parameter& p) {
    ImGui::PushID(p.getPath().generic_string().c_str());
    ImGui::BeginGroup();

    bool changed = ui::drawField(p, "##value");

    ImGui::EndGroup();
    if (p.getDescription() && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        ImGui::SetTooltip("%s", p.getDescription()->c_str());
    ImGui::PopID();

    if (changed) p.sync();
    return changed;
}

std::vector<ParameterItem> ParameterUI::buildItems(const ParameterPath& prefix) {
    ParameterRegistry& parameters = Core::getParameters();
    std::vector<ParameterItem> items;
    std::unordered_map<std::string, size_t> conditionGroups;

    for (const auto& param : parameters.getAll()) {
        if (param->getPath().parent_path() != prefix) continue;
        if (!param->getCondition()) {
            items.push_back({ .parameter = param.get() });
            continue;
        }
        const auto& cond = *param->getCondition();
        const std::string key = cond.param.string();
        if (!conditionGroups.contains(key)) {
            conditionGroups[key] = items.size();
            items.push_back({ .condition = cond });
        }
        items[conditionGroups.at(key)].children.push_back({ .parameter = param.get() });
    }

    std::vector<std::string> seen;
    for (const auto& param : parameters.getAll()) {
        const auto rel = param->getPath().lexically_relative(prefix);
        if (rel.empty()) continue;
        const std::string seg = rel.begin()->string();
        if (seg == "..") continue;
        if (std::next(rel.begin()) == rel.end()) continue;
        if (std::find(seen.begin(), seen.end(), seg) != seen.end()) continue;
        seen.push_back(seg);
        items.push_back({
            .path = prefix / seg,
            .collapsible = true,
            .children = buildItems(prefix / seg)
        });
    }

    return items;
}

void ParameterUI::drawItem(ParameterItem& item, bool& changed, bool& restartNeeded) {
    if (item.parameter) {
        if (drawParameter(*item.parameter)) {
            changed = true;
            if (item.parameter->isRestartingAnimation()) restartNeeded = true;
        }
        return;
    }

    if (item.collapsible) {
        ParameterRegistry& parameters = Core::getParameters();
        const auto& labels = parameters.getNodeLabels();
        auto it = labels.find(item.path.generic_string());
        const std::string& label = it != labels.end() ? it->second : item.path.filename().string();
        ImGui::SeparatorText(label.c_str());
    }

    bool conditionMet = !item.condition ||
        (Core::getParameters().get<bool>(item.condition->param) == item.condition->when);

    float lineX = ImGui::GetCursorScreenPos().x + ImGui::GetStyle().IndentSpacing * 0.5f;
    float startY = ImGui::GetCursorScreenPos().y;
    ImGui::Indent();
    if (!conditionMet) ImGui::BeginDisabled();
    for (auto& child : item.children)
        drawItem(child, changed, restartNeeded);
    if (!conditionMet) ImGui::EndDisabled();
    float endY = ImGui::GetCursorScreenPos().y - ImGui::GetStyle().ItemSpacing.y;
    ImGui::Unindent();
    ui::drawIndentLine(lineX, startY, endY);
}

ParameterUI& ParameterUI::get() {
    static ParameterUI instance;
    return instance;
}

void ParameterUI::drawGroup(const ParameterPath& root) {
    ParameterItem& cached = get().root;
    if (cached.children.empty())
        cached.children = buildItems("");

    for (auto& child : cached.children) {
        if (child.path != root) continue;
        bool changed = false, restartNeeded = false;
        for (auto& item : child.children)
            drawItem(item, changed, restartNeeded);
        if (restartNeeded) Core::markDirty();
        return;
    }
}
