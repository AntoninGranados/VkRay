#include "input_handler.hpp"

#include <algorithm>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "imgui/imgui.h"
#include "imgui/ImGuizmo.h"

#include "VkSmol/platform/platform.hpp"

#include "core/animation/animation_clock.hpp"
#include "core/camera/camera.hpp"
#include "core/core.hpp"
#include "core/ecs/systems/camera_system.hpp"
#include "core/scene/scene.hpp"
#include "editor/editor.hpp"
#include "editor/editor_ui.hpp"

bool InputHandler::isMouseInputBlocked() {
    if (ImGuizmo::IsUsing()) return true;
    if (!Core::getScene().getCamera().isLocked()) return false;
    return Editor::getUi().isMouseCaptured() || ImGui::GetIO().WantCaptureMouse;
}

void InputHandler::initCallbacks() {
    Core::getPlatform().setCursorPosCallback([](double x, double y){
        if (isMouseInputBlocked()) return;
        if (Core::getScene().getCamera().cursorPosCallback(x, y)) {
            Core::markDirty();
            ecs::syncPreviewCameraToEntity(Core::getScene().getCamera(), Core::getScene().getRegistry());
        }
    });

    Core::getPlatform().setScrollCallback([](double xoffset, double yoffset){
        if (ImGui::GetIO().WantCaptureMouse || Editor::getUi().isMouseCaptured()) return;
        if (Core::getRenderMode() != RenderMode::Preview) return;
        if (Core::getScene().getCamera().scrollCallback(xoffset, yoffset)) Core::markDirty();
    });
}

void InputHandler::pollEvents() {
    Core::getPlatform().pollEvents();
}

void InputHandler::handle(float dt) {
    switch (Core::getRenderMode()) {
        case RenderMode::Preview:           handlePreview(dt); break;
        case RenderMode::RenderSingle:
        case RenderMode::RenderAnimation:   handleRender(dt);  break;
    }
}

void InputHandler::handlePreview(float dt) {
    const bool blockMouseInput    = isMouseInputBlocked();
    const bool blockKeyboardInput = Editor::getUi().isKeyboardCaptured() || ImGui::GetIO().WantCaptureKeyboard;

    Core::getPlatform().setCursorMode(
        (Core::getScene().getCamera().isLocked() || blockMouseInput) ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED
    );

    if (Core::getPlatform().getKey(GLFW_KEY_ESCAPE)) {
        if (!Editor::getUi().isToggled()) Editor::getUi().toggle();
        else Editor::getUi().clearEntitySelection();
    }

    if (!blockKeyboardInput && justPressed(GLFW_KEY_TAB)) {
        if (Core::getScene().getCamera().hasPreviewCamera()) Editor::getUi().clearPreview();
        else Editor::getUi().setPreview();
    }

    if (!blockKeyboardInput && Core::getScene().getCamera().isLocked() && justPressed(GLFW_KEY_SPACE))
        Core::getAnimation().toggle();

    handleFrameStepKey(GLFW_KEY_LEFT, -1, dt, blockKeyboardInput);
    handleFrameStepKey(GLFW_KEY_RIGHT, 1, dt, blockKeyboardInput);

    if (!blockKeyboardInput) {
        if (Core::getScene().getCamera().processInput(dt)) {
            Core::markDirty();
            ecs::syncPreviewCameraToEntity(Core::getScene().getCamera(), Core::getScene().getRegistry());
        }
    }

    if (!blockKeyboardInput && Core::getPlatform().getKey(GLFW_KEY_R))
        Core::markDirty();
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
        Core::markDirty();
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
    Editor::getUi().restoreToggledState();
    Core::setRenderMode(RenderMode::Preview);
}
