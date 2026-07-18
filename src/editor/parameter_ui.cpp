#include "parameter_ui.hpp"

#include <algorithm>
#include <vector>

#include "imgui/imgui.h"

#include "core/core.hpp"

bool ParameterUI::drawParameter(ParameterBase& base) {
    bool changed = false;
    ImGui::PushID(base.path.generic_string().c_str());

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

    ImGui::PopID();
    if (changed && base.onSync) base.onSync();
    return changed;
}

bool ParameterUI::drawNode(ParameterHandler& handler, const ParameterPath& prefix, bool& accumulationRestartNeeded) {
    bool changed = false;
    const auto& parameters = handler.getParameterList();
    const auto& nodeLabels = handler.getNodeLabels();

    for (const auto& parameter : parameters) {
        if (parameter->path.parent_path() != prefix) continue;
        if (drawParameter(*parameter)) {
            changed = true;
            if (parameter->restartAccumulation) accumulationRestartNeeded = true;
        }
    }

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
