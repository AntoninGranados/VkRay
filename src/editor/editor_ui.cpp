#include "editor_ui.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"
#include "imgui/imgui_internal.h"
#include "imgui/ImGuizmo.h"

#include "VkSmol/engine.hpp"
#include "VkSmol/platform/glfw_platform.hpp"

#include "app/app_context.hpp"
#include "scene/scene.hpp"
#include "./ui_constants.hpp"


void EditorUi::draw(const CommandBuffer& commandBuffer, AppContext& ctx) {
    if (!toggled && ctx.renderMode == RenderMode::Preview) return;

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    
    ImGui::NewFrame();
    updateState();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = ui::kWidgetRounding;
    style.FrameRounding = ui::kWidgetRounding;

    if (ctx.renderMode != RenderMode::Preview) drawRender(ctx);
    else drawPreview(ctx);

    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer.get());
}

void EditorUi::updateState() {
    ImGuiIO& io = ImGui::GetIO();
    capturesMouse = io.WantCaptureMouse;
    capturesKeyboard = io.WantCaptureKeyboard;
}

void EditorUi::drawPreview(AppContext& ctx) {
    Camera& camera = *ctx.camera;
    Scene& scene = *ctx.scene;

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

        scene.runOnUi(ctx);

        const float aspect = windowSize.y > 0.0f ? windowSize.x / windowSize.y : 1.0f;
        scene.drawGuizmo(
            camera.getView(),
            camera.getProjection(aspect)
        );
    }
    ImGui::End();
    ImGui::PopStyleVar(2);

    statsPanel.draw(ctx);
    animationPanel.draw(ctx);
    cameraPanel.draw(ctx);
    renderParameterPanel.draw(ctx);
    scenePanel.draw(ctx);
    inspectorPanel.draw(ctx);
    toastPanel.draw();
}

void EditorUi::drawRender(AppContext& ctx) {
    renderPanel.draw(ctx);
}
