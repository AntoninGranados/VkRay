#include "parameter_ui.hpp"

#include <optional>
#include <string>
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

} // namespace

void drawGroup(const FieldPath& root) {
    static std::vector<ui::FieldGroup> cached;

    if (cached.empty()) {
        std::vector<Field*> ptrs;
        for (const auto& param : Core::getParameters().getAll()) ptrs.push_back(param.get());
        cached = ui::buildFieldGroups(ptrs);
        ui::clusterByCondition(cached, [](const FieldPath& id) -> std::optional<int> {
            for (const auto& param : Core::getParameters().getAll())
                if (param->getId() == id) return param->conditionValue();
            return std::nullopt;
        });
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
