#include "editor_ui.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"
#include "imgui/ImGuizmo.h"

#include "scene/scene.hpp"
#include "./ui_constants.hpp"

#include "engine/engine.hpp"
#include "engine/platform/platform.hpp"
#include "app/app_context.hpp"


void EditorUi::draw(const CommandBuffer& commandBuffer, AppContext& ctx) {
    if (!toggled && ctx.renderState->renderMode == RenderMode::Preview) return;

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    
    ImGui::NewFrame();
    updateState();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = ui::kWidgetRounding;
    style.FrameRounding = ui::kWidgetRounding;

    if (ctx.renderState->renderMode != RenderMode::Preview) drawRender(ctx);
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

        scene.drawGuizmo(
            camera.getView(),
            camera.getProjection(static_cast<GLFWwindow*>(ctx.platform->getNativeWindowHandle()))
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
    notificationPanel.draw(ctx);
}

void EditorUi::drawRender(AppContext& ctx) {
    renderPanel.draw(ctx);
}
