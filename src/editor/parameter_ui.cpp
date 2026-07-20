#include "parameter_ui.hpp"

#include <algorithm>
#include <unordered_map>
#include <vector>

#include <nfd.hpp>

#include "FontAwesome/IconsFontAwesome7.h"
#include "imgui/imgui.h"

#include "core/core.hpp"
#include "editor/ui_constants.hpp"

template <>
bool ParameterUI::drawParameter(IntParameter& p) {
    ImGui::Text("%s", p.label.c_str());
    ImGui::SetNextItemWidth(-FLT_MIN);
    return ImGui::DragInt("##value", &p.get(), static_cast<float>(p.getStep()), p.getMin(), p.getMax());
}

template <>
bool ParameterUI::drawParameter(FloatParameter& p) {
    ImGui::Text("%s", p.label.c_str());
    ImGui::SetNextItemWidth(-FLT_MIN);
    return ImGui::DragFloat("##value", &p.get(), p.getStep(), p.getMin(), p.getMax());
}

template <>
bool ParameterUI::drawParameter(BoolParameter& p) {
    return ImGui::Checkbox(p.label.c_str(), &p.get());
}

template <>
bool ParameterUI::drawParameter(EnumParameter& p) {
    std::vector<const char*> names;
    for (const auto& item : p.getItems()) names.push_back(item.c_str());
    ImGui::Text("%s", p.label.c_str());
    ImGui::SetNextItemWidth(-FLT_MIN);
    return ImGui::Combo("##value", &p.get(), names.data(), static_cast<int>(names.size()));
}

template <>
bool ParameterUI::drawParameter(PathParameter& p) {
    ImGui::Text("%s", p.label.c_str());
    std::string display = p.get().empty() ? "(not set)" : p.get().string();
    float browseWidth = ImGui::CalcTextSize(ICON_FA_FOLDER_OPEN).x + ImGui::GetStyle().FramePadding.x * 2.0f;
    ImGui::SetNextItemWidth(-browseWidth - ImGui::GetStyle().ItemSpacing.x);
    ImGui::BeginDisabled();
    ImGui::InputText("##path", display.data(), display.size() + 1, ImGuiInputTextFlags_ReadOnly);
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (!ImGui::Button(ICON_FA_FOLDER_OPEN "##browse", ImVec2(browseWidth, 0))) return false;

    NFD::Guard guard;
    NFD::UniquePath outPath;
    nfdresult_t result;
    if (p.getExtensions().empty()) {
        std::string defaultPath = p.get().string();
        result = NFD::PickFolder(outPath, defaultPath.empty() ? nullptr : defaultPath.c_str());
    } else {
        std::vector<std::pair<std::string, std::string>> extStrs;
        for (const auto& e : p.getExtensions())
            extStrs.push_back({ e.displayName(), e.ext });
        std::vector<nfdfilteritem_t> filters;
        for (const auto& [name, ext] : extStrs)
            filters.push_back({ name.c_str(), ext.c_str() });
        std::string defaultName = p.get().filename().string();
        result = NFD::SaveDialog(outPath, filters.data(), static_cast<nfdfiltersize_t>(filters.size()), nullptr, defaultName.empty() ? nullptr : defaultName.c_str());
    }
    if (result != NFD_OKAY) return false;
    p.get() = outPath.get();
    if (p.onSync) p.onSync();
    return true;
}

template <>
bool ParameterUI::drawParameter(ParameterBase& base) {
    bool changed = false;
    ImGui::PushID(base.path.generic_string().c_str());
    ImGui::BeginGroup();

    if      (IntParameter*   p = dynamic_cast<IntParameter*>(&base))   changed = drawParameter(*p);
    else if (FloatParameter* p = dynamic_cast<FloatParameter*>(&base)) changed = drawParameter(*p);
    else if (BoolParameter*  p = dynamic_cast<BoolParameter*>(&base))  changed = drawParameter(*p);
    else if (EnumParameter*  p = dynamic_cast<EnumParameter*>(&base))  changed = drawParameter(*p);
    else if (PathParameter*  p = dynamic_cast<PathParameter*>(&base))  changed = drawParameter(*p);

    ImGui::EndGroup();
    if (base.description && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        ImGui::SetTooltip("%s", base.description->c_str());
    ImGui::PopID();

    if (changed && base.onSync) base.onSync();
    return changed;
}

// TODO: refactor
std::vector<ParameterItem> ParameterUI::buildItems(const ParameterPath& prefix) {
    ParameterHandler& handler = Core::getParameters();
    std::vector<ParameterItem> items;
    std::unordered_map<std::string, size_t> conditionGroups;

    for (const auto& param : handler.getParameterList()) {
        if (param->path.parent_path() != prefix) continue;
        if (!param->condition) {
            items.push_back({ .parameter = param.get() });
            continue;
        }
        const auto& cond = *param->condition;
        const std::string key = cond.param.string();
        if (!conditionGroups.count(key)) {
            conditionGroups[key] = items.size();
            items.push_back({ .condition = cond });
        }
        items[conditionGroups.at(key)].children.push_back({ .parameter = param.get() });
    }

    std::vector<std::string> seen;
    for (const auto& param : handler.getParameterList()) {
        const auto rel = param->path.lexically_relative(prefix);
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
            if (item.parameter->restartAccumulation) restartNeeded = true;
        }
        return;
    }

    if (item.collapsible) {
        ParameterHandler& handler = Core::getParameters();
        const auto& labels = handler.getNodeLabels();
        auto it = labels.find(item.path.generic_string());
        const std::string& label = it != labels.end() ? it->second : item.path.filename().string();
        if (!ImGui::CollapsingHeader(label.c_str())) return;
    }

    bool conditionMet = !item.condition ||
        (Core::getParameters().get<bool>(item.condition->param) == item.condition->when);

    float lineX  = ImGui::GetCursorScreenPos().x + ImGui::GetStyle().IndentSpacing * 0.5f;
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
        if (restartNeeded) Core::requestAccumulationRestart();
        return;
    }
}
