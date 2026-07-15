#include "editor_ui.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "FontAwesome/IconsFontAwesome7.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"
#include "imgui/imgui_internal.h"

#include "core/core.hpp"
#include "core/scene/scene.hpp"
#include "ui_constants.hpp"

EditorUi::EditorUi() {
    viewportPanel.setOnEntitySelectionCallback(
        [this](int id) { selection.entity = id; }
    );
}

void EditorUi::draw(const CommandBuffer& commandBuffer) {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();

    ImGui::NewFrame();
    updateState();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = ui::kWidgetRounding;
    style.FrameRounding  = ui::kWidgetRounding;

    if (Core::getRenderMode() != RenderMode::Preview) drawRender();
    else if (toggled) drawPreview();

    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer.get());
}

void EditorUi::updateState() {
    ImGuiIO& io = ImGui::GetIO();
    if (viewportPanel.isHovered() && Core::getRenderMode() == RenderMode::Preview)
        io.WantCaptureMouse = false;
    capturesMouse    = io.WantCaptureMouse;
    capturesKeyboard = io.WantCaptureKeyboard;
}

void EditorUi::setupDockspace() {
    ImGuiID dockspace_id = ImGui::GetID("Dock space");
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::DockSpaceOverViewport(dockspace_id, ImGui::GetMainViewport(), ImGuiDockNodeFlags_None);

    ImGuiDockNode* dockNode = ImGui::DockBuilderGetNode(dockspace_id);
    if (dockNode == nullptr || dockNode->IsLeafNode()) {
        ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_None);
        ImGui::DockBuilderSetNodeSize(dockspace_id, vp->Size);

        ImGuiID dock_bottom, dock_right, dock_center;
        ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down,  0.15f, &dock_bottom, &dock_center);
        ImGui::DockBuilderSplitNode(dock_center,  ImGuiDir_Right, 0.25f, &dock_right,  &dock_center);

        ImGui::DockBuilderDockWindow("Animation",                dock_bottom);
        ImGui::DockBuilderDockWindow(ICON_FA_CAMERA " Renderer", dock_right);
        ImGui::DockBuilderDockWindow(ICON_FA_VIDEO " Camera",    dock_right);
        ImGui::DockBuilderDockWindow(ICON_FA_CUBES " Scene",     dock_right);
        ImGui::DockBuilderDockWindow("FPS",                      dock_right);
        ImGui::DockBuilderDockWindow("Viewport",                 dock_center);
        ImGui::DockBuilderFinish(dockspace_id);
    }
}

void EditorUi::drawPreview() {
    Scene& scene = Core::getScene();

    setupDockspace();
    viewportPanel.draw();

    statsPanel.draw();
    animationPanel.draw();
    scenePanel.draw();
    renderParameterPanel.draw();
    sceneUI.drawInspectors(scene, selection);
    toastNotifications.draw();
    debugPanel.draw();
}

void EditorUi::drawRender() {
    renderViewportPanel.draw();
    renderPanel.draw();
}

void EditorUi::clearEntitySelection() {
    selection.entity = -1;
}
