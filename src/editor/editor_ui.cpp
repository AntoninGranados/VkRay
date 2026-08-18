#include "editor_ui.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <optional>

#include "FontAwesome/IconsFontAwesome7.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"
#include "imgui/imgui_internal.h"

#include "core/core.hpp"
#include "core/scene/scene.hpp"
#include "editor/editor.hpp"
#include "ui_utils.hpp"

EditorUi::EditorUi() {
    initStyle();
    viewportPanel.setOnEntitySelectionCallback(
        [this](std::optional<ecs::Entity> id) {
            clearPreview();
            if (Editor::getSelectedEntity() != id) {
                if (id.has_value()) Editor::setSelectedEntity(*id);
                else Editor::clearSelectedEntity();
                Core::markDirty();
            }
        }
    );
}

void EditorUi::initStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding     = ImVec2(8.0f, 8.0f);
    style.FramePadding      = ImVec2(4.0f, 3.0f);
    style.ItemSpacing       = ImVec2(8.0f, 4.0f);
    style.ItemInnerSpacing  = ImVec2(4.0f, 4.0f);
    style.IndentSpacing     = 12.0f;
    style.ScrollbarSize     = 10.0f;
    style.GrabMinSize       = 10.0f;
    style.WindowBorderSize  = 1.0f;
    style.FrameBorderSize   = 0.0f;
    style.WindowRounding    = ui::kWidgetRounding;
    style.ChildRounding     = ui::kWidgetRounding;
    style.FrameRounding     = ui::kWidgetRounding;
    style.ScrollbarRounding = ui::kWidgetRounding;
    style.GrabRounding      = ui::kWidgetRounding;
    style.TabRounding       = ui::kWidgetRounding;
    style.DisabledAlpha     = 0.2f;

    ImVec4* c = style.Colors;
    c[ImGuiCol_Text]                      = ui::kDraculaFg;
    c[ImGuiCol_TextDisabled]              = ui::luma(ui::kDraculaFg, 0.4f);
    c[ImGuiCol_WindowBg]                  = ui::kDraculaBg;
    c[ImGuiCol_ChildBg]                   = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    c[ImGuiCol_PopupBg]                   = ui::kDraculaBg;
    c[ImGuiCol_Border]                    = ui::kDraculaSurface;
    c[ImGuiCol_BorderShadow]              = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    c[ImGuiCol_FrameBg]                   = ui::kDraculaSurface;
    c[ImGuiCol_FrameBgHovered]            = ImVec4(ui::kDraculaSurface.x, ui::kDraculaSurface.y, ui::kDraculaSurface.z, 0.7f);
    c[ImGuiCol_FrameBgActive]             = ui::kDraculaPurple;
    c[ImGuiCol_TitleBg]                   = ui::kDraculaBg;
    c[ImGuiCol_TitleBgActive]             = ui::kDraculaBg;
    c[ImGuiCol_TitleBgCollapsed]          = ui::kDraculaBg;
    c[ImGuiCol_MenuBarBg]                 = ui::kDraculaBg;
    c[ImGuiCol_ScrollbarBg]               = ui::kDraculaBg;
    c[ImGuiCol_ScrollbarGrab]             = ui::kDraculaSurface;
    c[ImGuiCol_ScrollbarGrabHovered]      = ui::kDraculaSubtle;
    c[ImGuiCol_ScrollbarGrabActive]       = ui::kDraculaPurple;
    c[ImGuiCol_CheckMark]                 = ui::kDraculaPurple;
    c[ImGuiCol_SliderGrab]                = ui::kDraculaPurple;
    c[ImGuiCol_SliderGrabActive]          = ui::kDraculaPink;
    c[ImGuiCol_Button]                    = ui::kDraculaSurface;
    c[ImGuiCol_ButtonHovered]             = ui::kDraculaPurple;
    c[ImGuiCol_ButtonActive]              = ui::kDraculaPink;
    c[ImGuiCol_Header]                    = ImVec4(ui::kDraculaPurple.x, ui::kDraculaPurple.y, ui::kDraculaPurple.z, 0.35f);
    c[ImGuiCol_HeaderHovered]             = ImVec4(ui::kDraculaPurple.x, ui::kDraculaPurple.y, ui::kDraculaPurple.z, 0.7f);
    c[ImGuiCol_HeaderActive]              = ui::kDraculaPurple;
    c[ImGuiCol_Separator]                 = ui::kDraculaSurface;
    c[ImGuiCol_SeparatorHovered]          = ui::kDraculaPurple;
    c[ImGuiCol_SeparatorActive]           = ui::kDraculaPink;
    c[ImGuiCol_ResizeGrip]                = ui::kDraculaSurface;
    c[ImGuiCol_ResizeGripHovered]         = ui::kDraculaPurple;
    c[ImGuiCol_ResizeGripActive]          = ui::kDraculaPink;
    c[ImGuiCol_Tab]                       = ui::kDraculaBg;
    c[ImGuiCol_TabHovered]                = ui::kDraculaPurple;
    c[ImGuiCol_TabSelected]               = ui::kDraculaSurface;
    c[ImGuiCol_TabSelectedOverline]       = ui::kDraculaPurple;
    c[ImGuiCol_TabDimmed]                 = ui::kDraculaBg;
    c[ImGuiCol_TabDimmedSelected]         = ui::kDraculaSurface;
    c[ImGuiCol_TabDimmedSelectedOverline] = ui::kDraculaSubtle;
    c[ImGuiCol_DockingPreview]            = ImVec4(ui::kDraculaPurple.x, ui::kDraculaPurple.y, ui::kDraculaPurple.z, 0.5f);
    c[ImGuiCol_DockingEmptyBg]            = ui::kDraculaBg;
    c[ImGuiCol_PlotLines]                 = ui::kDraculaFg;
    c[ImGuiCol_PlotLinesHovered]          = ui::kDraculaPink;
    c[ImGuiCol_PlotHistogram]             = ui::kDraculaPurple;
    c[ImGuiCol_PlotHistogramHovered]      = ui::kDraculaPink;
    c[ImGuiCol_TableHeaderBg]             = ui::kDraculaBg;
    c[ImGuiCol_TableBorderStrong]         = ui::kDraculaSurface;
    c[ImGuiCol_TableBorderLight]          = ui::kDraculaSurface;
    c[ImGuiCol_TableRowBg]                = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    c[ImGuiCol_TableRowBgAlt]             = ImVec4(ui::kDraculaSurface.x, ui::kDraculaSurface.y, ui::kDraculaSurface.z, 0.3f);
    c[ImGuiCol_TextLink]                  = ui::kDraculaCyan;
    c[ImGuiCol_TextSelectedBg]            = ImVec4(ui::kDraculaPurple.x, ui::kDraculaPurple.y, ui::kDraculaPurple.z, 0.35f);
    c[ImGuiCol_DragDropTarget]            = ui::kDraculaYellow;
    c[ImGuiCol_NavCursor]                 = ui::kDraculaPurple;
    c[ImGuiCol_NavWindowingHighlight]     = ImVec4(ui::kDraculaPurple.x, ui::kDraculaPurple.y, ui::kDraculaPurple.z, 0.7f);
    c[ImGuiCol_NavWindowingDimBg]         = ImVec4(ui::kDraculaBg.x, ui::kDraculaBg.y, ui::kDraculaBg.z, 0.7f);
    c[ImGuiCol_ModalWindowDimBg]          = ImVec4(ui::kDraculaBg.x, ui::kDraculaBg.y, ui::kDraculaBg.z, 0.7f);
}

void EditorUi::draw(const CommandBuffer& commandBuffer) {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();

    if (!Core::getScene().getCamera().isLocked())
        ImGui::GetIO().AddMousePosEvent(-FLT_MAX, -FLT_MAX);

    ImGui::NewFrame();
    updateState();

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

        ImGuiID dock_left, dock_right, dock_bottom, dock_center;
        ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down,  0.15f, &dock_bottom, &dock_center);
        ImGui::DockBuilderSplitNode(dock_center,  ImGuiDir_Left,  0.25f, &dock_left,   &dock_center);
        ImGui::DockBuilderSplitNode(dock_center,  ImGuiDir_Right, 0.30f, &dock_right,  &dock_center);

        ImGui::DockBuilderDockWindow("Animation",                dock_bottom);
        ImGui::DockBuilderDockWindow(ICON_FA_CAMERA " Renderer", dock_left);
        ImGui::DockBuilderDockWindow(ICON_FA_CUBES " Scene",     dock_left);
        ImGui::DockBuilderDockWindow("Inspector",                dock_right);
        ImGui::DockBuilderDockWindow("Viewport",                 dock_center);
        ImGui::DockBuilderFinish(dockspace_id);
    }
}

void EditorUi::setPreview() {
    const std::optional<ecs::Entity> selectedEntity = Editor::getSelectedEntity();
    if (!selectedEntity.has_value()) return;
    Scene& scene = Core::getScene();
    const ecs::Entity e = *selectedEntity;
    if (!scene.getRegistry().has(e, ecs::Camera)) return;
    scene.getCamera().setPreviewCamera(e);
    Core::markDirty();
}

void EditorUi::clearPreview() {
    if (Core::getScene().getCamera().clearPreviewCamera())
        Core::markDirty();
}

void EditorUi::drawPreview() {
    setupDockspace();
    viewportPanel.draw();

    statsPanel.draw();
    animationPanel.draw();
    scenePanel.draw();
    renderParameterPanel.draw();
    inspectorPanel.draw();
    toastNotifications.draw();
    debugPanel.draw();
}

void EditorUi::drawRender() {
    renderViewportPanel.draw();
    renderPanel.draw();
}

void EditorUi::clearEntitySelection() {
    clearPreview();
    Editor::clearSelectedEntity();
}
