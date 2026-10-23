#include "editor_ui.hpp"

#include <optional>

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "core/core.hpp"
#include "core/scene/scene.hpp"
#include "editor/ecs/components/camera.hpp"
#include "editor/editor.hpp"
#include "editor/panels/animation_panel.hpp"
#include "editor/panels/debug_panel.hpp"
#include "editor/panels/inspector_panel.hpp"
#include "editor/panels/render_progress_panel.hpp"
#include "editor/panels/render_viewport_panel.hpp"
#include "editor/panels/renderer_panel.hpp"
#include "editor/panels/scene_panel.hpp"
#include "editor/panels/stats_panel.hpp"
#include "editor/panels/toast_notifications.hpp"
#include "ui_utils.hpp"

EditorUi::EditorUi() {
    initStyle();

    viewportPanel = &addPanel<ViewportPanel>(previewPanels);
    addPanel<StatsPanel>(previewPanels);
    addPanel<AnimationPanel>(previewPanels);
    addPanel<RendererPanel>(previewPanels);
    addPanel<ScenePanel>(previewPanels);
    addPanel<InspectorPanel>(previewPanels);
    addPanel<ToastNotifications>(previewPanels);
    addPanel<DebugPanel>(previewPanels);

    addPanel<RenderViewportPanel>(renderPanels);
    addPanel<RenderProgressPanel>(renderPanels);

    viewportPanel->setOnEntitySelectionCallback([](std::optional<ecs::Entity> id) { Editor::selectEntity(id); });
}

void EditorUi::initStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(8.0f, 8.0f);
    style.FramePadding = ImVec2(4.0f, 3.0f);
    style.ItemSpacing = ImVec2(8.0f, 4.0f);
    style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
    style.IndentSpacing = 12.0f;
    style.ScrollbarSize = 10.0f;
    style.GrabMinSize = 10.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.WindowRounding = ui::kWidgetRounding;
    style.ChildRounding = ui::kWidgetRounding;
    style.FrameRounding = ui::kWidgetRounding;
    style.ScrollbarRounding = ui::kWidgetRounding;
    style.GrabRounding = ui::kWidgetRounding;
    style.TabRounding = ui::kWidgetRounding;
    style.DisabledAlpha = 0.2f;

    ImVec4* c = style.Colors;
    c[ImGuiCol_Text] = ui::kDraculaFg;
    c[ImGuiCol_TextDisabled] = ui::luma(ui::kDraculaFg, 0.4f);
    c[ImGuiCol_WindowBg] = ui::kDraculaBg;
    c[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    c[ImGuiCol_PopupBg] = ui::kDraculaBg;
    c[ImGuiCol_Border] = ui::kDraculaSurface;
    c[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    c[ImGuiCol_FrameBg] = ui::kDraculaSurface;
    c[ImGuiCol_FrameBgHovered] = ImVec4(ui::kDraculaSurface.x, ui::kDraculaSurface.y, ui::kDraculaSurface.z, 0.7f);
    c[ImGuiCol_FrameBgActive] = ui::kDraculaPurple;
    c[ImGuiCol_TitleBg] = ui::kDraculaBg;
    c[ImGuiCol_TitleBgActive] = ui::kDraculaBg;
    c[ImGuiCol_TitleBgCollapsed] = ui::kDraculaBg;
    c[ImGuiCol_MenuBarBg] = ui::kDraculaBg;
    c[ImGuiCol_ScrollbarBg] = ui::kDraculaBg;
    c[ImGuiCol_ScrollbarGrab] = ui::kDraculaSurface;
    c[ImGuiCol_ScrollbarGrabHovered] = ui::kDraculaSubtle;
    c[ImGuiCol_ScrollbarGrabActive] = ui::kDraculaPurple;
    c[ImGuiCol_CheckMark] = ui::kDraculaPurple;
    c[ImGuiCol_SliderGrab] = ui::kDraculaPurple;
    c[ImGuiCol_SliderGrabActive] = ui::kDraculaPink;
    c[ImGuiCol_Button] = ui::kDraculaSurface;
    c[ImGuiCol_ButtonHovered] = ui::kDraculaPurple;
    c[ImGuiCol_ButtonActive] = ui::kDraculaPink;
    c[ImGuiCol_Header] = ImVec4(ui::kDraculaPurple.x, ui::kDraculaPurple.y, ui::kDraculaPurple.z, 0.35f);
    c[ImGuiCol_HeaderHovered] = ImVec4(ui::kDraculaPurple.x, ui::kDraculaPurple.y, ui::kDraculaPurple.z, 0.7f);
    c[ImGuiCol_HeaderActive] = ui::kDraculaPurple;
    c[ImGuiCol_Separator] = ui::kDraculaSurface;
    c[ImGuiCol_SeparatorHovered] = ui::kDraculaPurple;
    c[ImGuiCol_SeparatorActive] = ui::kDraculaPink;
    c[ImGuiCol_ResizeGrip] = ui::kDraculaSurface;
    c[ImGuiCol_ResizeGripHovered] = ui::kDraculaPurple;
    c[ImGuiCol_ResizeGripActive] = ui::kDraculaPink;
    c[ImGuiCol_Tab] = ui::kDraculaBg;
    c[ImGuiCol_TabHovered] = ui::kDraculaPurple;
    c[ImGuiCol_TabSelected] = ui::kDraculaSurface;
    c[ImGuiCol_TabSelectedOverline] = ui::kDraculaPurple;
    c[ImGuiCol_TabDimmed] = ui::kDraculaBg;
    c[ImGuiCol_TabDimmedSelected] = ui::kDraculaSurface;
    c[ImGuiCol_TabDimmedSelectedOverline] = ui::kDraculaSubtle;
    c[ImGuiCol_DockingPreview] = ImVec4(ui::kDraculaPurple.x, ui::kDraculaPurple.y, ui::kDraculaPurple.z, 0.5f);
    c[ImGuiCol_DockingEmptyBg] = ui::kDraculaBg;
    c[ImGuiCol_PlotLines] = ui::kDraculaFg;
    c[ImGuiCol_PlotLinesHovered] = ui::kDraculaPink;
    c[ImGuiCol_PlotHistogram] = ui::kDraculaPurple;
    c[ImGuiCol_PlotHistogramHovered] = ui::kDraculaPink;
    c[ImGuiCol_TableHeaderBg] = ui::kDraculaBg;
    c[ImGuiCol_TableBorderStrong] = ui::kDraculaSurface;
    c[ImGuiCol_TableBorderLight] = ui::kDraculaSurface;
    c[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    c[ImGuiCol_TableRowBgAlt] = ImVec4(ui::kDraculaSurface.x, ui::kDraculaSurface.y, ui::kDraculaSurface.z, 0.3f);
    c[ImGuiCol_TextLink] = ui::kDraculaCyan;
    c[ImGuiCol_TextSelectedBg] = ImVec4(ui::kDraculaPurple.x, ui::kDraculaPurple.y, ui::kDraculaPurple.z, 0.35f);
    c[ImGuiCol_DragDropTarget] = ui::kDraculaYellow;
    c[ImGuiCol_NavCursor] = ui::kDraculaPurple;
    c[ImGuiCol_NavWindowingHighlight] = ImVec4(ui::kDraculaPurple.x, ui::kDraculaPurple.y, ui::kDraculaPurple.z, 0.7f);
    c[ImGuiCol_NavWindowingDimBg] = ImVec4(ui::kDraculaBg.x, ui::kDraculaBg.y, ui::kDraculaBg.z, 0.7f);
    c[ImGuiCol_ModalWindowDimBg] = ImVec4(ui::kDraculaBg.x, ui::kDraculaBg.y, ui::kDraculaBg.z, 0.7f);
}

void EditorUi::draw() {
    const ecs::Registry& registry = Core::getScene().getRegistry();
    const ecs::Entity camera = *Core::getScene().getActiveCamera();
    if (!registry.get(camera, ecs::CameraNavigation).payload<ecs::CameraNavigationState>("state").locked)
        ImGui::GetIO().AddMousePosEvent(-FLT_MAX, -FLT_MAX);

    Editor::getImGui().newFrame();
    updateState();

    const bool preview = Core::getRenderMode() == RenderMode::Preview;
    if (preview) setupDockspace();
    for (const std::unique_ptr<Panel>& panel : preview ? previewPanels : renderPanels) panel->draw();
}

void EditorUi::updateState() {
    ImGuiIO& io = ImGui::GetIO();
    if (viewportPanel->isHovered() && Core::getRenderMode() == RenderMode::Preview) io.WantCaptureMouse = false;
    capturesMouse = io.WantCaptureMouse;
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
        ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.15f, &dock_bottom, &dock_center);
        ImGui::DockBuilderSplitNode(dock_center, ImGuiDir_Left, 0.25f, &dock_left, &dock_center);
        ImGui::DockBuilderSplitNode(dock_center, ImGuiDir_Right, 0.30f, &dock_right, &dock_center);

        for (const std::unique_ptr<Panel>& panel : previewPanels) {
            const DockSlot slot = panel->getDockSlot();
            if (slot == DockSlot::None) continue;
            ImGuiID node = dock_center;
            if (slot == DockSlot::Left) node = dock_left;
            if (slot == DockSlot::Right) node = dock_right;
            if (slot == DockSlot::Bottom) node = dock_bottom;
            ImGui::DockBuilderDockWindow(panel->getTitle().c_str(), node);
        }
        ImGui::DockBuilderFinish(dockspace_id);
    }
}
