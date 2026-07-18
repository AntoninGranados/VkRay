#include "debug_panel.hpp"

#include "imgui/imgui.h"

#include "core/core.hpp"
#include "core/core_renderer.hpp"
#include "core/parameter_handler.hpp"
#include "core/structures.hpp"
#include "editor/editor.hpp"

void DebugPanel::content() {
    if (Core::getParameters().getEnum<DebugView>("render/debug_view") == DebugView::None)
        return;

    ImGui::SetNextWindowSize(ImVec2(400.0f, 300.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Debug View", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        VkExtent2D extent = Core::getCoreRenderer().getRenderExtent();
        if (extent.width == 0 || extent.height == 0) { ImGui::End(); return; }
        float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);

        ImVec2 avail = ImGui::GetContentRegionAvail();
        ImVec2 imageSize;
        if (avail.x / avail.y > aspect)
            imageSize = ImVec2(avail.y * aspect, avail.y);
        else
            imageSize = ImVec2(avail.x, avail.x / aspect);

        ImVec2 cursor = ImGui::GetCursorPos();
        ImGui::SetCursorPos(ImVec2(cursor.x + (avail.x - imageSize.x) * 0.5f, cursor.y + (avail.y - imageSize.y) * 0.5f));
        ImGui::Image(Editor::getEditorRenderer().getDebugTexId(), imageSize);
    }
    ImGui::End();
}
