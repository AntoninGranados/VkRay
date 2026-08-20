#include "render_viewport_panel.hpp"

#include "imgui/imgui.h"

#include "core/core.hpp"
#include "editor/editor.hpp"
#include "editor/ui_utils.hpp"

void RenderViewportPanel::content() {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->Pos);
    ImGui::SetNextWindowSize(vp->Size);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ui::kDraculaBg);
    ImGui::Begin("RenderView", nullptr,
        ImGuiWindowFlags_NoDecoration         |
        ImGuiWindowFlags_NoMove               |
        ImGuiWindowFlags_NoMouseInputs        |
        ImGuiWindowFlags_NoBringToFrontOnFocus
    );

    VkExtent2D renderExtent = Core::getCoreRenderer().getRenderExtent();
    ui::drawFittedImage(Editor::getEditorRenderer().getOutputTexId(),
        ImVec2(static_cast<float>(renderExtent.width), static_cast<float>(renderExtent.height)));

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}
