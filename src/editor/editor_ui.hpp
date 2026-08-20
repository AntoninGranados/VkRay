#pragma once

#include "VkSmol/command/command_buffer.hpp"

#include "editor/panels/animation_panel.hpp"
#include "editor/panels/debug_panel.hpp"
#include "editor/panels/inspector_panel.hpp"
#include "editor/panels/render_progress_panel.hpp"
#include "editor/panels/render_viewport_panel.hpp"
#include "editor/panels/renderer_panel.hpp"
#include "editor/panels/scene_panel.hpp"
#include "editor/panels/stats_panel.hpp"
#include "editor/panels/toast_notifications.hpp"
#include "editor/panels/viewport_panel.hpp"

class Scene;
class CommandBuffer;

class EditorUi {
public:
    EditorUi();
    void draw(const CommandBuffer& commandBuffer);

    bool isToggled() { return toggled; }
    void toggle() { toggled = !toggled; }
    void setToggle(bool newToggle) { toggled = newToggle; }
    void saveToggledState() { toggleState = toggled; }
    void restoreToggledState() { toggled = toggleState; }

    bool isMouseCaptured()    { return capturesMouse; }
    bool isKeyboardCaptured() { return capturesKeyboard; }

    void clearEntitySelection();
    void setPreview();
    void clearPreview();

    ImVec2      getViewportSize()            const { return viewportPanel.getSize(); }
    ImVec2      getViewportPos()             const { return viewportPanel.getPos(); }
    ImDrawList* getViewportDrawList()        const { return viewportPanel.getDrawList(); }

private:
    StatsPanel          statsPanel;
    AnimationPanel      animationPanel;
    RenderProgressPanel renderPanel;
    RendererPanel       renderParameterPanel;
    ScenePanel          scenePanel;
    InspectorPanel      inspectorPanel;
    DebugPanel          debugPanel;
    ViewportPanel       viewportPanel;
    RenderViewportPanel renderViewportPanel;
    ToastNotifications  toastNotifications;

    bool toggled      = true;
    bool toggleState  = true;
    bool capturesMouse    = false;
    bool capturesKeyboard = false;

    static void initStyle();
    void setupDockspace();
    void updateState();
    void drawPreview();
    void drawRender();
};
