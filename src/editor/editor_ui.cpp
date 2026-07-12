#include "editor_ui.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"
#include "imgui/imgui_internal.h"
#include "imgui/ImGuizmo.h"

#include "core/core.hpp"
#include "core/scene/scene.hpp"
#include "editor/ecs/systems/camera_drawing_system.hpp"
#include "ui_constants.hpp"


EditorUi::EditorUi() {
    uiScheduler.add(ecs::cameraDrawingSystem);
}

void EditorUi::draw(const CommandBuffer& commandBuffer) {
    if (!toggled && Core::getRenderMode() == RenderMode::Preview) return;

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();

    ImGui::NewFrame();
    updateState();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = ui::kWidgetRounding;
    style.FrameRounding = ui::kWidgetRounding;

    if (Core::getRenderMode() != RenderMode::Preview) drawRender();
    else drawPreview();

    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer.get());
}

void EditorUi::updateState() {
    ImGuiIO& io = ImGui::GetIO();
    capturesMouse = io.WantCaptureMouse;
    capturesKeyboard = io.WantCaptureKeyboard;
}

void EditorUi::drawPreview() {
    Camera& camera = Core::getScene().getCamera();
    Scene& scene = Core::getScene();

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::AllowAxisFlip(false);
    ImGuizmo::BeginFrame();
    
    ImGuiID dockspace_id = ImGui::GetID("Dock space");
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode;
    ImGui::DockSpaceOverViewport(dockspace_id, ImGui::GetMainViewport(), dockspaceFlags);

    if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr) {
        ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_PassthruCentralNode);
        ImGui::DockBuilderSetNodeSize(dockspace_id, vp->Size);

        ImGuiID dock_bottom;
        ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.15f, &dock_bottom, &dockspace_id);
        ImGui::DockBuilderDockWindow("Animation", dock_bottom);
        ImGui::DockBuilderFinish(dockspace_id);
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("Gizmo View", nullptr, ImGuiWindowFlags_NoBackground | 
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoMouseInputs |
        ImGuiWindowFlags_NoDocking
    );
    {
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 windowSize = ImGui::GetWindowSize();
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(windowPos.x, windowPos.y, windowSize.x, windowSize.y);

        uiScheduler.run(scene.getRegistry());

        const float aspect = windowSize.y > 0.0f ? windowSize.x / windowSize.y : 1.0f;
        sceneUI.drawGuizmo(
            scene,
            selection,
            camera.getView(),
            camera.getProjection(aspect)
        );
    }
    ImGui::End();
    ImGui::PopStyleVar(2);

    statsPanel.draw();
    animationPanel.draw();
    cameraPanel.draw();
    renderParameterPanel.draw();
    scenePanel.draw();
    sceneUI.drawInspectors(scene, selection);
    toastNotifications.draw();
}

void EditorUi::drawRender() {
    renderPanel.draw();
}

void EditorUi::pickEntity(Scene& scene, const glm::vec2& screenPos, const glm::vec2& screenSize) {
    float dist;
    selection.entity = sceneUI.raycast(scene, screenPos, screenSize, dist);
}

bool EditorUi::focusDepthAt(Scene& scene, const glm::vec2& screenPos, const glm::vec2& screenSize, float& dist) {
    return sceneUI.raycast(scene, screenPos, screenSize, dist, false) >= 0;
}

void EditorUi::clearEntitySelection() {
    selection.entity = -1;
}
