#include "render_viewport_panel.hpp"

#include "imgui/imgui.h"

#include "core/core.hpp"
#include "editor/editor.hpp"

void RenderViewportPanel::draw() {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->Pos);
    ImGui::SetNextWindowSize(vp->Size);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    ImGui::Begin("RenderView", nullptr,
        ImGuiWindowFlags_NoDecoration         |
        ImGuiWindowFlags_NoMove               |
        ImGuiWindowFlags_NoMouseInputs        |
        ImGuiWindowFlags_NoBringToFrontOnFocus
    );

    ImVec2 available = ImGui::GetContentRegionAvail();
    ImVec2 imageSize = available;

    VkExtent2D renderExtent = Core::getCoreRenderer().getRenderExtent();
    if (renderExtent.width > 0 && renderExtent.height > 0) {
        const float renderAspect    = static_cast<float>(renderExtent.width) / renderExtent.height;
        const float availableAspect = available.x / available.y;

        if (availableAspect > renderAspect) {
            imageSize.y = available.y;
            imageSize.x = imageSize.y * renderAspect;
        } else {
            imageSize.x = available.x;
            imageSize.y = imageSize.x / renderAspect;
        }
    }

    ImVec2 cursor = ImGui::GetCursorPos();
    ImGui::SetCursorPos({
        cursor.x + (available.x - imageSize.x) * 0.5f,
        cursor.y + (available.y - imageSize.y) * 0.5f
    });
    ImGui::Image(Editor::getEditorRenderer().getDisplayTexId(), imageSize);

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}
