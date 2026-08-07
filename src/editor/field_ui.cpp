#include "field_ui.hpp"

#include <filesystem>
#include <string>
#include <vector>

#include "FontAwesome/IconsFontAwesome7.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "imgui/imgui.h"
#include <nfd.hpp>

namespace ui {

bool drawField(Field& field, const std::string& widgetId) {
    const FieldMetadata& metadata = field.getMetadata();
    const float step = metadata.step != 0.0f ? metadata.step : 0.01f;
    const float fmin = metadata.min, fmax = metadata.max;
    auto toInt = [](float f) -> int {
        if (f <= float(std::numeric_limits<int>::min())) return std::numeric_limits<int>::min();
        if (f >= float(std::numeric_limits<int>::max())) return std::numeric_limits<int>::max();
        return static_cast<int>(f);
    };

    bool changed = false;
    const bool hasPresets = !metadata.presets.empty();

    if (hasPresets) {
        ImGui::Text("%s", field.getLabel().c_str());

        const int presetIdx = field.findPreset();
        float maxLabelW = 0.0f;
        for (const auto& p : metadata.presets)
            maxLabelW = std::max(maxLabelW, ImGui::CalcTextSize(p.label.c_str()).x);

        const char* preview = presetIdx >= 0 ? metadata.presets[presetIdx].label.c_str() : "\xe2\x80\x94";
        float comboW = maxLabelW + ImGui::GetStyle().FramePadding.x * 2.0f + ImGui::GetFrameHeight();

        ImGui::SetNextItemWidth(comboW);
        if (ImGui::BeginCombo(("##pre" + widgetId).c_str(), preview)) {
            for (int i = 0; i < (int)metadata.presets.size(); i++) {
                bool sel = (i == presetIdx);
                if (ImGui::Selectable(metadata.presets[i].label.c_str(), sel)) {
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
            glm::vec2 v = field.get<glm::vec2>();
            bool c = ImGui::DragFloat2(widgetId.c_str(), glm::value_ptr(v), step, fmin, fmax);
            if (c) { field.set<glm::vec2>(v); changed = true; }
            return changed;
        }
        case FieldType::Vec3: {
            setup();
            glm::vec3 v = field.get<glm::vec3>();
            bool c = ImGui::DragFloat3(widgetId.c_str(), glm::value_ptr(v), step, fmin, fmax);
            if (c) { field.set<glm::vec3>(v); changed = true; }
            return changed;
        }
        case FieldType::Vec4: {
            setup();
            glm::vec4 v = field.get<glm::vec4>();
            bool c = ImGui::DragFloat4(widgetId.c_str(), glm::value_ptr(v), step, fmin, fmax);
            if (c) { field.set<glm::vec4>(v); changed = true; }
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
            for (const auto& item : metadata.enumItems) names.push_back(item.c_str());
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

            NFD::Guard guard;
            NFD::UniquePath outPath;
            nfdresult_t result;
            if (metadata.pathExtensions.empty()) {
                result = NFD::PickFolder(outPath, current.empty() ? nullptr : current.c_str());
            } else {
                std::vector<std::pair<std::string, std::string>> extStrs;
                for (const auto& e : metadata.pathExtensions)
                    extStrs.push_back({ e.displayName(), e.ext });
                std::vector<nfdfilteritem_t> filters;
                for (const auto& [name, ext] : extStrs)
                    filters.push_back({ name.c_str(), ext.c_str() });
                if (metadata.pathSave) {
                    std::string defaultName = std::filesystem::path(current).filename().string();
                    result = NFD::SaveDialog(outPath, filters.data(), (nfdfiltersize_t)filters.size(),
                        nullptr, defaultName.empty() ? nullptr : defaultName.c_str());
                } else {
                    std::string defaultDir = current.empty() ? "" : std::filesystem::path(current).parent_path().string();
                    result = NFD::OpenDialog(outPath, filters.data(), (nfdfiltersize_t)filters.size(),
                        defaultDir.empty() ? nullptr : defaultDir.c_str());
                }
            }
            if (result != NFD_OKAY) return changed;
            field.set<std::filesystem::path>(std::filesystem::path(outPath.get()));
            return true;
        }
    }
    return changed;
}

}
