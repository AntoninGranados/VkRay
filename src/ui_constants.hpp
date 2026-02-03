#pragma once

#include <imgui/imgui.h>

namespace ui {

// Window
inline const float window_bg_alpha = 0.8f;

// Widget
inline const float widget_rounding = 3.0f;

// Button
inline const ImVec2 button_size = ImVec2(200.0f, 0.0f);

// Cancel Button
inline const ImVec4 cancel_col_button = ImVec4(0.8f, 0.15f, 0.15f, 1.0f);
inline const ImVec4 cancel_col_button_hovered = ImVec4(0.9f, 0.25f, 0.25f, 1.0f);
inline const ImVec4 cancel_col_button_active = ImVec4(0.7f, 0.1f, 0.1f, 1.0f);
inline void PushCancelStyleColor() {
    ImGui::PushStyleColor(ImGuiCol_Button, cancel_col_button);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, cancel_col_button_hovered);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, cancel_col_button_active);
}
inline void PopCancelStyleColor() {
    ImGui::PopStyleColor(3);
}

// Keyframe
inline const ImVec4 keyframe_col = ImVec4(0.8f, 0.15f, 0.15f, 1.0f);


}   // namespace ui
