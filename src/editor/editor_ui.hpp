#pragma once

#include "VkSmol/command/command_buffer.hpp"

#include "panels/animation_panel.hpp"
#include "panels/debug_panel.hpp"
#include "panels/render_progress_panel.hpp"
#include "panels/render_viewport_panel.hpp"
#include "panels/renderer_panel.hpp"
#include "panels/scene_panel.hpp"
#include "panels/stats_panel.hpp"
#include "panels/viewport_panel.hpp"
#include "scene/scene_ui.hpp"
#include "toast_notifications.hpp"

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

    const SceneSelection& getSelection()     const { return selection; }
    ImVec2      getViewportSize()            const { return viewportPanel.getSize(); }
    ImVec2      getViewportPos()             const { return viewportPanel.getPos(); }
    ImDrawList* getViewportDrawList()        const { return viewportPanel.getDrawList(); }

private:
    SceneSelection      selection;

    StatsPanel          statsPanel;
    AnimationPanel      animationPanel;
    RenderProgressPanel renderPanel;
    RendererPanel       renderParameterPanel;
    ScenePanel          scenePanel{selection};
    DebugPanel          debugPanel;
    ViewportPanel       viewportPanel;
    RenderViewportPanel renderViewportPanel;
    SceneUI             sceneUI;
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
