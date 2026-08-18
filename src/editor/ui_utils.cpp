#include "ui_utils.hpp"

#include "FontAwesome/IconsFontAwesome7.h"
#include "imgui/imgui.h"

#include "core/animation/animation_store.hpp"
#include "core/core.hpp"
#include "core/scene/scene.hpp"

namespace ui {

namespace {

void drawKeyframeIcon(bool has, bool hovered, bool active) {
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

} // namespace

void drawKeyframeButton(ecs::Entity e, ecs::Component& c, const std::string& fieldId) {
    AnimationStore& store = Core::getScene().getAnimationStore();
    const int frame = Core::getAnimation().getFrame();
    const bool has = store.has(e, c.getType(), fieldId, frame);

    constexpr float iconSize = 10.0f;
    ImGui::PushID((fieldId + "_keyframe").c_str());
    const bool clicked = ImGui::InvisibleButton("##keyframe", ImVec2(iconSize, ImGui::GetTextLineHeight()));
    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();
    ImGui::PopID();

    if (clicked) {
        if (has) store.remove(e, c.getType(), fieldId, frame);
        else store.capture(e, c, fieldId, frame);
        Core::markDirty();
    }

    drawKeyframeIcon(has, hovered, active);
    ImGui::SameLine();
}


}   // namespace ui
