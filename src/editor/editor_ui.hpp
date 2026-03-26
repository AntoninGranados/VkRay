#pragma once

#include "imgui/imgui.h"

#include "app/app_context.hpp"
#include "engine/engine.hpp"

#include "panels/stats_panel.hpp"
#include "panels/animation_panel.hpp"
#include "panels/render_panel.hpp"
#include "panels/notification_panel.hpp"
#include "panels/inspector_panel.hpp"
#include "panels/camera_panel.hpp"
#include "panels/render_parameter_panel.hpp"
#include "panels/scene_panel.hpp"

class EditorUi {
public:
    void draw(CommandBuffer commandBuffer, AppContext& ctx);

    bool isToggled() { return toggled; }
    void toggle() { toggled = !toggled; }
    void setToggle(bool newToggle) { toggled = newToggle; }
    void saveToggledState() { toggleState = toggled; }
    void restorToggledState() { toggled = toggleState; }

    bool isMouseCaptured() { return capturesMouse; }
    bool isKeyboardCaptured() { return capturesKeyboard; }

private:
    StatsPanel statsPanel;
    AnimationPanel animationPanel;
    RenderPanel renderPanel;
    NotificationPanel notificationPanel;
    InspectorPanel inspectorPanel;
    CameraPanel cameraPanel;
    RenderParameterPanel renderParameterPanel;
    ScenePanel scenePanel;
    
    bool toggled = true;
    bool toggleState = true;
    bool capturesMouse = false;
    bool capturesKeyboard = false;

    void updateState();

    void drawPreview(AppContext& ctx);
    void drawRender(AppContext& ctx);
};
