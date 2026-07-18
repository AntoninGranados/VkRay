#include "parameter_ui.hpp"

#include <algorithm>
#include <vector>

#include "imgui/imgui.h"

#include "core/core.hpp"

bool ParameterUI::drawParameter(ParameterBase& base) {
    bool changed = false;
    ImGui::PushID(base.path.generic_string().c_str());

    ImGui::BeginGroup();
    if (auto* p = dynamic_cast<IntParameter*>(&base)) {
        ImGui::Text("%s", base.label.c_str());
        ImGui::SetNextItemWidth(-FLT_MIN);
        changed = ImGui::DragInt("##value", &p->get(), static_cast<float>(p->getStep()), p->getMin(), p->getMax());
    } else if (auto* p = dynamic_cast<FloatParameter*>(&base)) {
        ImGui::Text("%s", base.label.c_str());
        ImGui::SetNextItemWidth(-FLT_MIN);
        changed = ImGui::DragFloat("##value", &p->get(), p->getStep(), p->getMin(), p->getMax());
    } else if (auto* p = dynamic_cast<BoolParameter*>(&base)) {
        changed = ImGui::Checkbox(base.label.c_str(), &p->get());
    } else if (auto* p = dynamic_cast<EnumParameter*>(&base)) {
        std::vector<const char*> names;
        for (const auto& item : p->getItems()) names.push_back(item.c_str());
        ImGui::Text("%s", base.label.c_str());
        ImGui::SetNextItemWidth(-FLT_MIN);
        changed = ImGui::Combo("##value", &p->get(), names.data(), static_cast<int>(names.size()));
    }
    ImGui::EndGroup();

    if (base.description && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        ImGui::SetTooltip("%s", base.description->c_str());

    ImGui::PopID();
    if (changed && base.onSync) base.onSync();
    return changed;
}

bool ParameterUI::drawNode(ParameterHandler& handler, const ParameterPath& prefix, bool& accumulationRestartNeeded) {
    bool changed = false;
    const auto& parameters = handler.getParameterList();
    const auto& nodeLabels = handler.getNodeLabels();

    std::string activeConditionGroup;
    float groupLineX  = 0.0f;
    float groupStartY = 0.0f;

    auto endConditionGroup = [&] {
        if (activeConditionGroup.empty()) return;
        float endY = ImGui::GetCursorScreenPos().y - ImGui::GetStyle().ItemSpacing.y;
        ImGui::Unindent();
        ImGui::GetWindowDrawList()->AddLine(
            { groupLineX, groupStartY },
            { groupLineX, endY },
            ImGui::GetColorU32(ImGuiCol_TextDisabled, 0.5f),
            1.5f
        );
        activeConditionGroup.clear();
    };

    for (const auto& parameter : parameters) {
        if (parameter->path.parent_path() != prefix) continue;
        if (parameter->condition) {
            const auto& cond = *parameter->condition;
            if (handler.getBool(cond.param) != cond.when) continue;
            std::string condKey = cond.param.generic_string();
            if (condKey != activeConditionGroup) {
                endConditionGroup();
                activeConditionGroup = condKey;
                groupLineX  = ImGui::GetCursorScreenPos().x + ImGui::GetStyle().IndentSpacing * 0.5f;
                groupStartY = ImGui::GetCursorScreenPos().y;
                ImGui::Indent();
            }
        } else {
            endConditionGroup();
        }
        
        if (drawParameter(*parameter)) {
            changed = true;
            if (parameter->restartAccumulation) accumulationRestartNeeded = true;
        }
    }
    endConditionGroup();

    std::vector<std::string> seen;
    for (const auto& parameter : parameters) {
        auto rel = parameter->path.lexically_relative(prefix);
        if (rel.empty()) continue;
        auto it = rel.begin();
        std::string seg = it->string();
        if (seg == "..") continue;
        it++;
        if (it == rel.end()) continue;
        if (std::find(seen.begin(), seen.end(), seg) != seen.end()) continue;
        seen.push_back(seg);

        auto labelIt = nodeLabels.find((prefix / seg).generic_string());
        const std::string& displayLabel = labelIt != nodeLabels.end() ? labelIt->second : seg;
        if (ImGui::CollapsingHeader(displayLabel.c_str())) {
            changed |= drawNode(handler, prefix / seg, accumulationRestartNeeded);
        }
    }

    return changed;
}

void ParameterUI::drawGroup(ParameterHandler& handler, const ParameterPath& root) {
    bool accumulationRestartNeeded = false;
    drawNode(handler, root, accumulationRestartNeeded);
    if (accumulationRestartNeeded) Core::requestAccumulationRestart();
}
