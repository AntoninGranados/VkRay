// TODO: find a better name for this file
#pragma once

#include <algorithm>
#include <cmath>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace ui {

// Dracula Theme
// https://draculatheme.com/contribute#color-palette
inline float srgbChannel(float c) noexcept {
    return c <= 0.04045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
}
inline ImVec4 fromSRGB(float r, float g, float b, float a = 1.0f) noexcept {
    return { srgbChannel(r), srgbChannel(g), srgbChannel(b), a };
}
inline ImVec4 luma(const ImVec4& c, float factor) noexcept {
    float h, s, v;
    ImGui::ColorConvertRGBtoHSV(c.x, c.y, c.z, h, s, v);
    v = std::clamp(v * factor, 0.0f, 1.0f);
    ImVec4 out { 0.0f, 0.0f, 0.0f, c.w };
    ImGui::ColorConvertHSVtoRGB(h, s, v, out.x, out.y, out.z);
    return out;
}

inline const ImVec4 kDraculaBg      = fromSRGB(0.157f, 0.165f, 0.212f); // #282a36
inline const ImVec4 kDraculaSurface = fromSRGB(0.267f, 0.278f, 0.353f); // #44475a
inline const ImVec4 kDraculaFg      = fromSRGB(0.973f, 0.973f, 0.949f); // #f8f8f2
inline const ImVec4 kDraculaSubtle  = fromSRGB(0.384f, 0.447f, 0.643f); // #6272a4
inline const ImVec4 kDraculaCyan    = fromSRGB(0.545f, 0.914f, 0.992f); // #8be9fd
inline const ImVec4 kDraculaGreen   = fromSRGB(0.314f, 0.980f, 0.482f); // #50fa7b
inline const ImVec4 kDraculaOrange  = fromSRGB(1.000f, 0.722f, 0.424f); // #ffb86c
inline const ImVec4 kDraculaPink    = fromSRGB(1.000f, 0.475f, 0.776f); // #ff79c6
inline const ImVec4 kDraculaPurple  = fromSRGB(0.741f, 0.576f, 0.976f); // #bd93f9
inline const ImVec4 kDraculaRed     = fromSRGB(1.000f, 0.333f, 0.333f); // #ff5555
inline const ImVec4 kDraculaYellow  = fromSRGB(0.945f, 0.980f, 0.549f); // #f1fa8c

// Window
inline const float kWindowBgAlpha = 0.95f;

// Widget
inline const float kWidgetRounding = 3.0f;

// Button
inline const ImVec2 kButtonSize = ImVec2(200.0f, 0.0f);

// Cancel Button
inline const ImVec4 kCancelButtonColor        = kDraculaRed;
inline const ImVec4 kCancelButtonHoveredColor = kDraculaPink;
inline const ImVec4 kCancelButtonActiveColor  = luma(kDraculaRed, 0.7f);
inline void PushCancelStyleColor() {
    ImGui::PushStyleColor(ImGuiCol_Button, kCancelButtonColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kCancelButtonHoveredColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, kCancelButtonActiveColor);
}
inline void PopCancelStyleColor() {
    ImGui::PopStyleColor(3);
}

// Transparent Button
inline const ImVec4 kTransparentButtonColor = ImVec4();
inline const ImVec4 kTransparentButtonHoveredColor = ImVec4();
inline const ImVec4 kTransparentButtonActiveColor = ImVec4();
inline void PushTransparentStyleColor() {
    ImGui::PushStyleColor(ImGuiCol_Button, kTransparentButtonColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kTransparentButtonHoveredColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, kTransparentButtonActiveColor);
}
inline void PopTransparentStyleColor() {
    ImGui::PopStyleColor(3);
}

// Keyframe
inline const ImVec4 kKeyframeOffColor = kDraculaSubtle;
inline const ImVec4 kKeyframeOnColor  = kDraculaOrange;

inline void drawIndentLine(float x, float startY, float endY) {
    ImGui::GetWindowDrawList()->AddLine(
        { x, startY },
        { x, endY },
        ImGui::GetColorU32(ImGuiCol_TextDisabled, 0.5f),
        1.5f
    );
}

// Call before Begin to pin a window to its dock node
inline void setNextWindowFixed(bool noTabBar = false) {
    static ImGuiWindowClass fixedClass;
    fixedClass.DockNodeFlagsOverrideSet = static_cast<ImGuiDockNodeFlags>(
        static_cast<int>(ImGuiDockNodeFlags_NoUndocking)
        | ImGuiDockNodeFlags_NoWindowMenuButton
        | (noTabBar ? ImGuiDockNodeFlags_NoTabBar : 0)
    );
    ImGui::SetNextWindowClass(&fixedClass);
}

}   // namespace ui
