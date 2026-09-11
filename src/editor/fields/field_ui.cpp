#include "field_ui.hpp"

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

#include "FontAwesome/IconsFontAwesome7.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "core/fields/field.hpp"
#include "imgui/imgui.h"

#include "core/core.hpp"
#include "core/scene/scene.hpp"
#include "editor/ui_utils.hpp"

namespace ui {

namespace {
std::vector<ecs::Entity> findEntityCandidates(Scene& scene, ecs::Registry& registry, const EntityMeta* entityMeta) {
    std::vector<ecs::Entity> candidates;
    for (const ecs::Entity root : {scene.getMaterialsRoot(), scene.getAssetsRoot(), scene.getObjectsRoot()}) {
        for (const ecs::Entity child : scene.getChildren(root)) {
            if (entityMeta && !entityMeta->needs.empty()) {
                bool passes = true;
                for (const std::string& needed : entityMeta->needs) {
                    const auto found = ecs::ComponentType::find(needed);
                    if (!found || !registry.has(child, found->get())) { passes = false; break; }
                }
                if (!passes) continue;
            }
            if (entityMeta && !entityMeta->conflicts.empty()) {
                bool blocked = false;
                for (const std::string& conflicted : entityMeta->conflicts) {
                    const auto found = ecs::ComponentType::find(conflicted);
                    if (found && registry.has(child, found->get())) { blocked = true; break; }
                }
                if (blocked) continue;
            }
            candidates.push_back(child);
        }
    }
    return candidates;
}

void drawLinkButton(Field& field) {
    ImGui::PushID((field.getId().string() + "_link").c_str());
    const float size = ImGui::GetFrameHeight();
    const char* icon = field.isLinked() ? ICON_FA_LINK : ICON_FA_LINK_SLASH;

    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const bool clicked = ImGui::InvisibleButton("##link", ImVec2(size, size));
    const bool hovered = ImGui::IsItemHovered();
    const ImVec2 rectMax(pos.x + size, pos.y + size);
    ImGui::GetWindowDrawList()->AddRectFilled(
        pos, rectMax,
        ImGui::GetColorU32(hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button), ImGui::GetStyle().FrameRounding);

    ui::drawCenteredIcon(icon, pos, rectMax, ImGui::GetColorU32(ImGuiCol_Text));

    if (clicked) field.setLinked(!field.isLinked());
    ImGui::PopID();
    ImGui::SameLine();
}

template<typename Vec>
Vec linkedRatio(const Vec& oldV, const Vec& newV) {
    for (int i = 0; i < oldV.length(); i++) {
        if (newV[i] != oldV[i]) {
            const float ratio = (oldV[i] != 0.0f) ? newV[i] / oldV[i] : 1.0f;
            return oldV * ratio;
        }
    }
    return newV;
}
} // namespace

bool drawField(Field& field, const std::string& widgetId) {
    const FieldMetadata& metadata = field.getMetadata();
    const NumericMeta* numMeta = std::get_if<NumericMeta>(&metadata);
    const PathMeta* pathMeta = std::get_if<PathMeta>(&metadata);
    const EnumMeta* enumMeta = std::get_if<EnumMeta>(&metadata);
    const EntityMeta* entityMeta = std::get_if<EntityMeta>(&metadata);

    const float step = numMeta && numMeta->step != 0.0f ? numMeta->step : 0.01f;
    const float fmin = numMeta ? numMeta->min : -std::numeric_limits<float>::infinity();
    const float fmax = numMeta ? numMeta->max : std::numeric_limits<float>::infinity();
    auto toInt = [](float f) -> int {
        if (f <= float(std::numeric_limits<int>::min())) return std::numeric_limits<int>::min();
        if (f >= float(std::numeric_limits<int>::max())) return std::numeric_limits<int>::max();
        return static_cast<int>(f);
    };

    bool changed = false;

    const std::vector<FieldPreset>* presets = field.getPresets();
    const bool hasPresets = presets != nullptr && !presets->empty();

    if (field.isAnimatable()) ui::drawKeyframeButton(field);

    if (hasPresets) {
        ImGui::Text("%s", field.getLabel().c_str());

        const int presetIdx = field.findPreset();
        float maxLabelW = 0.0f;
        for (const auto& p : *presets)
            maxLabelW = std::max(maxLabelW, ImGui::CalcTextSize(p.label.c_str()).x);

        const char* preview = presetIdx >= 0 ? (*presets)[presetIdx].label.c_str() : "\xe2\x80\x94";
        float comboW = maxLabelW + ImGui::GetStyle().FramePadding.x * 2.0f + ImGui::GetFrameHeight();

        ImGui::SetNextItemWidth(comboW);
        if (ImGui::BeginCombo(("##pre" + widgetId).c_str(), preview)) {
            for (int i = 0; i < (int)presets->size(); i++) {
                bool sel = (i == presetIdx);
                if (ImGui::Selectable((*presets)[i].label.c_str(), sel)) {
                    field.applyPreset(i);
                    changed = true;
                }
                if (sel) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-FLT_MIN);
    }

    auto setup = [&] {
        if (hasPresets) return;
        ImGui::Text("%s", field.getLabel().c_str());
        ImGui::SetNextItemWidth(-FLT_MIN);
    };

    switch (field.getType()) {
        case FieldType::Bool: {
            bool v = field.get<bool>();
            bool c = ImGui::Checkbox(field.getLabel().c_str(), &v);
            if (c) { field.set<bool>(v); changed = true; }
            return changed;
        }
        case FieldType::Int: {
            setup();
            int v = field.get<int>();
            bool c = ImGui::DragInt(widgetId.c_str(), &v, step, toInt(fmin), toInt(fmax));
            if (c) { field.set<int>(v); changed = true; }
            return changed;
        }
        case FieldType::IVec2: {
            setup();
            glm::ivec2 v = field.get<glm::ivec2>();
            bool c = ImGui::DragInt2(widgetId.c_str(), glm::value_ptr(v), step, toInt(fmin), toInt(fmax));
            if (c) { field.set<glm::ivec2>(v); changed = true; }
            return changed;
        }
        case FieldType::IVec3: {
            setup();
            glm::ivec3 v = field.get<glm::ivec3>();
            bool c = ImGui::DragInt3(widgetId.c_str(), glm::value_ptr(v), step, toInt(fmin), toInt(fmax));
            if (c) { field.set<glm::ivec3>(v); changed = true; }
            return changed;
        }
        case FieldType::IVec4: {
            setup();
            glm::ivec4 v = field.get<glm::ivec4>();
            bool c = ImGui::DragInt4(widgetId.c_str(), glm::value_ptr(v), step, toInt(fmin), toInt(fmax));
            if (c) { field.set<glm::ivec4>(v); changed = true; }
            return changed;
        }
        case FieldType::Float: {
            setup();
            float v = field.get<float>();
            bool c = ImGui::DragFloat(widgetId.c_str(), &v, step, fmin, fmax);
            if (c) { field.set<float>(v); changed = true; }
            return changed;
        }
        case FieldType::Vec2: {
            setup();
            const glm::vec2 oldV = field.get<glm::vec2>();
            glm::vec2 v = oldV;
            if (numMeta && numMeta->linkable) drawLinkButton(field);
            ImGui::SetNextItemWidth(-FLT_MIN);
            bool c = ImGui::DragFloat2(widgetId.c_str(), glm::value_ptr(v), step, fmin, fmax);
            if (c) {
                if (numMeta && numMeta->linkable && field.isLinked()) v = linkedRatio(oldV, v);
                field.set<glm::vec2>(v);
                changed = true;
            }
            return changed;
        }
        case FieldType::Vec3: {
            setup();
            const glm::vec3 oldV = field.get<glm::vec3>();
            glm::vec3 v = oldV;
            bool c;
            bool linkable = false;
            if (numMeta && numMeta->color) {
                c = ImGui::ColorEdit3(widgetId.c_str(), glm::value_ptr(v));
            } else {
                linkable = numMeta && numMeta->linkable;
                if (linkable) drawLinkButton(field);
                ImGui::SetNextItemWidth(-FLT_MIN);
                c = ImGui::DragFloat3(widgetId.c_str(), glm::value_ptr(v), step, fmin, fmax);
            }
            if (c) {
                if (linkable && field.isLinked()) v = linkedRatio(oldV, v);
                field.set<glm::vec3>(v);
                changed = true;
            }
            return changed;
        }
        case FieldType::Vec4: {
            setup();
            const glm::vec4 oldV = field.get<glm::vec4>();
            glm::vec4 v = oldV;
            if (numMeta && numMeta->linkable) drawLinkButton(field);
            ImGui::SetNextItemWidth(-FLT_MIN);
            bool c = ImGui::DragFloat4(widgetId.c_str(), glm::value_ptr(v), step, fmin, fmax);
            if (c) {
                if (numMeta && numMeta->linkable && field.isLinked()) v = linkedRatio(oldV, v);
                field.set<glm::vec4>(v);
                changed = true;
            }
            return changed;
        }
        case FieldType::String: {
            setup();
            std::string v = field.get<std::string>();
            v.resize(Field::maxStringSize, '\0');
            bool c = ImGui::InputText(widgetId.c_str(), v.data(), Field::maxStringSize);
            if (c) { field.set<std::string>(v.c_str()); changed = true; }
            return changed;
        }
        case FieldType::Enum: {
            setup();
            int v = field.get<int>();
            std::vector<const char*> names;
            if (enumMeta) for (const auto& item : enumMeta->items) names.push_back(item.c_str());
            bool c = ImGui::Combo(widgetId.c_str(), &v, names.data(), (int)names.size());
            if (c) { field.set<int>(v); changed = true; }
            return changed;
        }
        case FieldType::Quat: {
            setup();
            glm::quat q = field.get<glm::quat>();
            glm::vec3 euler = glm::degrees(glm::eulerAngles(q));
            if (ImGui::DragFloat3(widgetId.c_str(), glm::value_ptr(euler), step)) {
                field.set<glm::quat>(glm::quat(glm::radians(euler)));
                changed = true;
            }
            return changed;
        }
        case FieldType::Path: {
            setup();
            std::string current = field.get<std::filesystem::path>().string();
            float browseWidth = ImGui::CalcTextSize(ICON_FA_FOLDER_OPEN).x + ImGui::GetStyle().FramePadding.x * 2.0f;
            ImGui::SetNextItemWidth(-browseWidth - ImGui::GetStyle().ItemSpacing.x);
            ImGui::BeginDisabled();
            std::string display = current.empty() ? "(not set)" : current;
            ImGui::InputText("##path", display.data(), display.size() + 1, ImGuiInputTextFlags_ReadOnly);
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (!ImGui::Button(ICON_FA_FOLDER_OPEN "##browse", ImVec2(browseWidth, 0))) return changed;

            std::optional<std::filesystem::path> result;
            if (!pathMeta || pathMeta->extensions.empty()) {
                result = pickFolderDialog(current);
            } else {
                std::vector<FileFilter> filters;
                for (const auto& e : pathMeta->extensions)
                    filters.push_back({ e.displayName(), e.ext });
                if (pathMeta->save) {
                    std::string defaultName = std::filesystem::path(current).filename().string();
                    result = saveFileDialog(filters, {}, defaultName);
                } else {
                    std::filesystem::path defaultDir = current.empty() ? std::filesystem::path{} : std::filesystem::path(current).parent_path();
                    result = openFileDialog(filters, defaultDir);
                }
            }
            if (!result) return changed;
            field.set<std::filesystem::path>(*result);
            return true;
        }
        case FieldType::Entity: {
            Scene& scene = Core::getScene();
            ecs::Registry& registry = scene.getRegistry();
            const std::vector<ecs::Entity> candidates = findEntityCandidates(scene, registry, entityMeta);

            const ecs::Entity current = field.get<ecs::Entity>();
            const auto found = std::find(candidates.begin(), candidates.end(), current);
            int currentIdx = (found != candidates.end()) ? static_cast<int>(found - candidates.begin()) : -1;

            auto getName = [&](ecs::Entity e) -> std::string {
                if (registry.has(e, ecs::Name))
                    return registry.get(e, ecs::Name).get<std::string>("value");
                return "Entity";
            };

            setup();
            const std::string preview = (currentIdx >= 0) ? getName(current) : "(none)";
            if (ImGui::BeginCombo(widgetId.c_str(), preview.c_str())) {
                for (int i = 0; i < (int)candidates.size(); i++) {
                    const bool selected = (i == currentIdx);
                    std::string label = getName(candidates[i]) + "##ent" + std::to_string(i);
                    if (ImGui::Selectable(label.c_str(), selected)) {
                        field.set<ecs::Entity>(candidates[i]);
                        changed = true;
                    }
                    if (selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            return changed;
        }
    }
    return changed;
}

std::vector<FieldGroup> buildFieldGroups(const std::vector<Field*>& fields, const FieldPath& prefix) {
    std::vector<FieldGroup> groups;

    for (Field* field : fields) {
        if (field->getId().parent_path() != prefix) continue;
        groups.push_back({ .field = field });
    }

    std::vector<std::string> seen;
    for (Field* field : fields) {
        const auto rel = field->getId().lexically_relative(prefix);
        if (rel.empty()) continue;
        const std::string seg = rel.begin()->string();
        if (seg == "..") continue;
        if (std::next(rel.begin()) == rel.end()) continue;
        if (std::find(seen.begin(), seen.end(), seg) != seen.end()) continue;
        seen.push_back(seg);
        groups.push_back({
            .id = prefix / seg,
            .children = buildFieldGroups(fields, prefix / seg)
        });
    }

    return groups;
}

bool drawFieldGroups(std::vector<FieldGroup>& groups, const std::string& widgetId,
                      const DrawFieldLeaf& drawLeaf, const FieldGroupLabel& label) {
    bool changed = false;

    for (auto& item : groups) {
        if (item.field) {
            changed |= drawLeaf(*item.field, widgetId);
            continue;
        }

        if (item.showHeader) {
            const std::string text = label ? label(item.id) : item.id.filename().string();
            ImGui::SeparatorText(text.c_str());
        }

        const float lineX = ImGui::GetCursorScreenPos().x + ImGui::GetStyle().IndentSpacing * 0.5f;
        const float startY = ImGui::GetCursorScreenPos().y;
        ImGui::Indent();

        const bool disabled = item.disabledWhen && item.disabledWhen();
        if (disabled) ImGui::BeginDisabled();
        changed |= drawFieldGroups(item.children, widgetId, drawLeaf, label);
        if (disabled) ImGui::EndDisabled();

        const float endY = ImGui::GetCursorScreenPos().y - ImGui::GetStyle().ItemSpacing.y;
        ImGui::Unindent();
        ui::drawIndentLine(lineX, startY, endY);
    }

    return changed;
}

bool drawGroupedFields(std::vector<Field>& fields, const std::string& widgetId) {
    std::vector<Field*> ptrs;
    ptrs.reserve(fields.size());
    for (Field& field : fields) ptrs.push_back(&field);

    auto groups = buildFieldGroups(ptrs);
    return drawFieldGroups(groups, widgetId, [](Field& field, const std::string& id) {
        return drawField(field, std::format("{}##{}", id, field.getId().string()));
    });
}

}
