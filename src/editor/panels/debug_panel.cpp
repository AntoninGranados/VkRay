#include "debug_panel.hpp"

#include "imgui/imgui.h"

#include "core/core.hpp"
#include "core/parameters/parameters.hpp"
#include "core/core_renderer.hpp"
#include "core/structures.hpp"
#include "editor/editor.hpp"
#include "editor/ui_utils.hpp"

void DebugPanel::content() {
    if (Core::getParameters().get<DebugView>("renderer/debug_view") == DebugView::None)
        return;

    ImGui::SetNextWindowSize(ImVec2(400.0f, 300.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Debug View", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        VkExtent2D extent = Core::getCoreRenderer().getRenderExtent();
        if (extent.width == 0 || extent.height == 0) { ImGui::End(); return; }

        ui::drawFittedImage(Editor::getEditorRenderer().getDebugTexId(),
            ImVec2(static_cast<float>(extent.width), static_cast<float>(extent.height)));
    }
    ImGui::End();
}
