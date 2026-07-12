#pragma once

#include <glm/glm.hpp>

#include "VkSmol/command/command_buffer.hpp"

#include "core/ecs/system_scheduler.hpp"
#include "panels/stats_panel.hpp"
#include "panels/animation_panel.hpp"
#include "panels/render_progress_panel.hpp"
#include "panels/camera_panel.hpp"
#include "panels/renderer_panel.hpp"
#include "panels/scene_panel.hpp"
#include "toast_notifications.hpp"
#include "scene/scene_ui.hpp"

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

    bool isMouseCaptured() { return capturesMouse; }
    bool isKeyboardCaptured() { return capturesKeyboard; }

    void pickEntity(Scene& scene, const glm::vec2& screenPos, const glm::vec2& screenSize);
    bool focusDepthAt(Scene& scene, const glm::vec2& screenPos, const glm::vec2& screenSize, float& dist);
    void clearEntitySelection();

    const SceneSelection& getSelection() const { return selection; }

private:
    ecs::SystemScheduler<> uiScheduler;

    SceneSelection selection;

    StatsPanel statsPanel;
    AnimationPanel animationPanel;
    RenderProgressPanel renderPanel;
    CameraPanel cameraPanel;
    RendererPanel renderParameterPanel;
    ScenePanel scenePanel{selection};
    SceneUI sceneUI;
    ToastNotifications toastNotifications;
    
    bool toggled = true;
    bool toggleState = true;
    bool capturesMouse = false;
    bool capturesKeyboard = false;

    void updateState();

    void drawPreview();
    void drawRender();
};
