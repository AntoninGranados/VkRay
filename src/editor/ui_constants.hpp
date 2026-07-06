#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace ui {

// Window
inline const float kWindowBgAlpha = 0.95f;
inline const float kWindowBgAlphaLight = 0.9f;

// Widget
inline const float kWidgetRounding = 3.0f;

// Button
inline const ImVec2 kButtonSize = ImVec2(200.0f, 0.0f);

// Cancel Button
inline const ImVec4 kCancelButtonColor = ImVec4(0.8f, 0.15f, 0.15f, 1.0f);
inline const ImVec4 kCancelButtonHoveredColor = ImVec4(0.9f, 0.25f, 0.25f, 1.0f);
inline const ImVec4 kCancelButtonActiveColor = ImVec4(0.7f, 0.1f, 0.1f, 1.0f);
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
inline const ImVec4 kKeyframeOffColor = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
inline const ImVec4 kKeyframeOnColor  = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);

// Call before SetNextWindowBgAlpha + Begin to pin a window to its dock node
inline void setFixedDockClass() {
    static ImGuiWindowClass fixedClass;
    fixedClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoUndocking
                                        | ImGuiDockNodeFlags_NoDockingSplit
                                        | ImGuiDockNodeFlags_NoWindowMenuButton;
    ImGui::SetNextWindowClass(&fixedClass);
}

}   // namespace ui
