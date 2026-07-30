#include "ui_utils.hpp"

#include "FontAwesome/IconsFontAwesome7.h"
#include "imgui/imgui.h"

#include "core/animation/animation_store.hpp"
#include "core/core.hpp"
#include "core/scene/scene.hpp"

namespace ui {

static void drawKeyframeIcon(bool has, bool hovered, bool active) {
    ImVec4 color;
    if (has) {
        if (active) color = luma(kKeyframeOnColor, 0.7f);
        else if (hovered) color = luma(kKeyframeOnColor, 1.3f);
        else color = kKeyframeOnColor;
    } else {
        color = ImGui::GetStyleColorVec4(hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
    }

    ImFont* font = ImGui::GetFont();
    const ImVec2 rectMin = ImGui::GetItemRectMin();
    const ImVec2 rectMax = ImGui::GetItemRectMax();
    constexpr float iconSize = 10.0f;
    const ImVec2 textSize = font->CalcTextSizeA(iconSize, FLT_MAX, 0.0f, ICON_FA_SQUARE);
    const ImVec2 pos = ImVec2(rectMin.x + (rectMax.x - rectMin.x - textSize.x) * 0.5f,
                              rectMin.y + (rectMax.y - rectMin.y - textSize.y) * 0.5f);
    ImGui::GetWindowDrawList()->AddText(font, iconSize, pos,
        ImGui::ColorConvertFloat4ToU32(color), ICON_FA_SQUARE);
}

void drawKeyframeButton(ecs::Entity e, ecs::Component& c, const ecs::Field& field) {
    AnimationStore& store = Core::getScene().getAnimationStore();
    const int frame = Core::getAnimation().getFrame();
    const Track& track = store.getTrack(e, c.getType(), field);
    const bool has = track.hasKeyframe(frame);

    constexpr float iconSize = 10.0f;
    ImGui::PushID((field.id + "_keyframe").c_str());
    const bool clicked = ImGui::InvisibleButton("##keyframe", ImVec2(iconSize, ImGui::GetTextLineHeight()));
    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();
    ImGui::PopID();

    if (clicked) {
        if (has) store.getTrack(e, c.getType(), field).removeKeyframe(frame);
        else store.getTrack(e, c.getType(), field).setKeyframe(frame, AnimationStore::sampleValue(c, field));
        Core::requestAccumulationRestart();
    }

    drawKeyframeIcon(has, hovered, active);
    ImGui::SameLine();
}

void drawKeyframeButton(MaterialHandle handle, const std::string& field) {
    AnimationStore& store = Core::getScene().getAnimationStore();
    const int frame = Core::getAnimation().getFrame();
    const Track& track = store.getTrack(handle, field);
    const bool has = track.hasKeyframe(frame);

    constexpr float iconSize = 10.0f;
    ImGui::PushID((field + "_keyframe").c_str());
    const bool clicked = ImGui::InvisibleButton("##keyframe", ImVec2(iconSize, ImGui::GetTextLineHeight()));
    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();
    ImGui::PopID();

    if (clicked) {
        if (has) store.getTrack(handle, field).removeKeyframe(frame);
        else {
            const Material& mat = Core::getScene().getMaterials()[handle];
            store.getTrack(handle, field).setKeyframe(frame, AnimationStore::sampleValue(mat, field));
        }
        Core::requestAccumulationRestart();
    }

    drawKeyframeIcon(has, hovered, active);
    ImGui::SameLine();
}

}   // namespace ui
