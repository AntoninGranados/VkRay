#pragma once

#include "app/app_context.hpp"

#include "VkSmol/command/command_buffer.hpp"
#include "panels/stats_panel.hpp"
#include "panels/animation_panel.hpp"
#include "panels/render_progress_panel.hpp"
#include "panels/inspector_panel.hpp"
#include "panels/camera_panel.hpp"
#include "panels/renderer_panel.hpp"
#include "panels/scene_panel.hpp"
#include "panels/toast_panel.hpp"

class CommandBuffer;

class EditorUi {
public:
    void draw(const CommandBuffer& commandBuffer, AppContext& ctx);

    bool isToggled() { return toggled; }
    void toggle() { toggled = !toggled; }
    void setToggle(bool newToggle) { toggled = newToggle; }
    void saveToggledState() { toggleState = toggled; }
    void restoreToggledState() { toggled = toggleState; }

    bool isMouseCaptured() { return capturesMouse; }
    bool isKeyboardCaptured() { return capturesKeyboard; }

private:
    StatsPanel statsPanel;
    AnimationPanel animationPanel;
    RenderProgressPanel renderPanel;
    InspectorPanel inspectorPanel;
    CameraPanel cameraPanel;
    RendererPanel renderParameterPanel;
    ScenePanel scenePanel;
    ToastPanel toastPanel;
    
    bool toggled = true;
    bool toggleState = true;
    bool capturesMouse = false;
    bool capturesKeyboard = false;

    void updateState();

    void drawPreview(AppContext& ctx);
    void drawRender(AppContext& ctx);
};
