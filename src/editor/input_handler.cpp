#include "input_handler.hpp"

#include <algorithm>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "imgui/imgui.h"
#include "imgui/ImGuizmo.h"

#include "VkSmol/platform/platform.hpp"

#include "core/animation/animation_clock.hpp"
#include "core/core.hpp"
#include "core/ecs/systems/camera_system.hpp"
#include "core/ecs/components/component.hpp"
#include "core/scene/scene.hpp"
#include "editor/editor.hpp"
#include "editor/editor_ui.hpp"
#include "editor/ecs/components/camera.hpp"
#include "editor/ecs/systems/camera_system.hpp"

bool InputHandler::isMouseInputBlocked() {
    if (ImGuizmo::IsUsing()) return true;

    const ecs::Registry& registry = Core::getScene().getRegistry();
    const ecs::Entity camera = Core::getScene().getCamera();
    if (!registry.get(camera, ecs::CameraNavigation).payload<ecs::CameraNavigationState>("state").locked)
        return false;

    return Editor::getUi().isMouseCaptured() || ImGui::GetIO().WantCaptureMouse;
}

void InputHandler::initCallbacks() {
    Core::getPlatform().setCursorPosCallback([](double x, double y){
        if (isMouseInputBlocked()) return;
        ecs::cameraCursorCallback(Core::getScene().getRegistry(), Core::getScene().getCamera(), x, y);
    });

    Core::getPlatform().setScrollCallback([](double xoffset, double yoffset){
        if (ImGui::GetIO().WantCaptureMouse || Editor::getUi().isMouseCaptured()) return;
        ecs::cameraScrollCallback(Core::getScene().getRegistry(), Core::getScene().getCamera(), xoffset, yoffset);
    });
}

void InputHandler::pollEvents() {
    ecs::cameraActivationSystem(Core::getScene().getRegistry());
    Core::getPlatform().pollEvents();
}

void InputHandler::handle(float dt) {
    switch (Core::getRenderMode()) {
        case RenderMode::Preview:         handlePreview(dt); break;
        case RenderMode::RenderSingle:
        case RenderMode::RenderAnimation: handleRender(dt);  break;
    }
}

void InputHandler::handlePreview(float dt) {
    const bool blockMouseInput    = isMouseInputBlocked();
    const bool blockKeyboardInput = Editor::getUi().isKeyboardCaptured() || ImGui::GetIO().WantCaptureKeyboard;

    const ecs::Component& ac = Core::getScene().getRegistry().get(Core::getScene().getCamera(), ecs::CameraNavigation);
    const ecs::CameraNavigationState& cameraState = ac.payload<ecs::CameraNavigationState>("state");

    Core::getPlatform().setCursorMode(
        (cameraState.locked || blockMouseInput) ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED
    );

    if (Core::getPlatform().getKey(GLFW_KEY_ESCAPE))
        Editor::selectEntity(std::nullopt);

    if (!blockKeyboardInput && justPressed(GLFW_KEY_TAB)) {
        if (Core::getScene().isPreviewing()) {
            Core::getScene().resetActiveCamera();
        } else if (const auto selected = Editor::getSelectedEntity(); selected && Core::getScene().getRegistry().has(*selected, ecs::Camera)) {
            Core::getScene().setActiveCamera(*selected);
        }
    }

    if (!blockKeyboardInput && cameraState.locked && justPressed(GLFW_KEY_SPACE))
        Core::getAnimation().toggle();

    handleFrameStepKey(GLFW_KEY_LEFT, -1, dt, blockKeyboardInput);
    handleFrameStepKey(GLFW_KEY_RIGHT, 1, dt, blockKeyboardInput);

    if (!blockKeyboardInput) {
        ecs::cameraActivationSystem(Core::getScene().getRegistry());
        ecs::cameraControlSystem(Core::getScene().getRegistry());
    }

    if (!blockKeyboardInput && Core::getPlatform().getKey(GLFW_KEY_R))
        Core::markRenderDirty();
}

bool InputHandler::justPressed(int key) {
    const bool down = Core::getPlatform().getKey(key);
    const bool first = down && !prevKeys[key];
    prevKeys[key] = down;
    return first;
}

void InputHandler::handleFrameStepKey(int key, int direction, float dt, bool blocked) {
    const bool pressed = !blocked && Core::getPlatform().getKey(key);
    if (!pressed) {
        prevKeys[key] = false;
        repeatTimers[key] = 0.0f;
        return;
    }

    AnimationClock& anim = Core::getAnimation();
    const auto step = [&]() {
        anim.reset(std::clamp(anim.getFrame() + direction, 0, anim.getEndFrame() - 1));
        anim.pause();
        Core::markRenderDirty();
    };

    if (!prevKeys[key]) {
        step();
        repeatTimers[key] = kFrameStepRepeatDelay;
    } else {
        repeatTimers[key] -= dt;
        if (repeatTimers[key] <= 0.0f) {
            step();
            repeatTimers[key] += kFrameStepRepeatInterval;
        }
    }
    prevKeys[key] = true;
}

void InputHandler::handleRender(float dt) {
    Core::getPlatform().setCursorMode(GLFW_CURSOR_NORMAL);
    if (Core::getPlatform().getKey(GLFW_KEY_ESCAPE)) {
        returnToPreview();
    }
}

void InputHandler::returnToPreview() {
    Core::setRenderMode(RenderMode::Preview);
}
