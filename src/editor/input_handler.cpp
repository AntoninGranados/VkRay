#include "input_handler.hpp"

#include <algorithm>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "imgui/imgui.h"
#include "imgui/ImGuizmo.h"

#include "VkSmol/platform/platform.hpp"

#include "core/animation/animation_handler.hpp"
#include "core/camera/camera.hpp"
#include "core/core.hpp"
#include "core/ecs/systems/camera_system.hpp"
#include "core/scene/scene.hpp"
#include "editor/editor.hpp"
#include "editor/editor_ui.hpp"

void InputHandler::initCallbacks() {
    Core::getPlatform().setCursorPosCallback([](double x, double y){
        const bool cameraLocked = Core::getScene().getCamera().isLocked();
        if (cameraLocked && (ImGui::GetIO().WantCaptureMouse || Editor::getUi().isMouseCaptured() || ImGuizmo::IsUsing()))
            return;
        if (Core::getScene().getCamera().cursorPosCallback(x, y)) {
            Core::requestAccumulationRestart();
            ecs::syncPreviewCameraToEntity(Core::getScene().getCamera(), Core::getScene().getRegistry());
        }
    });

    Core::getPlatform().setScrollCallback([](double xoffset, double yoffset){
        if (ImGui::GetIO().WantCaptureMouse || Editor::getUi().isMouseCaptured()) return;
        if (Core::getRenderMode() != RenderMode::Preview) return;
        if (Core::getScene().getCamera().scrollCallback(xoffset, yoffset)) Core::requestAccumulationRestart();
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
    const bool blockMouseInput    = ImGuizmo::IsUsing() || (Core::getScene().getCamera().isLocked() && (Editor::getUi().isMouseCaptured() || ImGui::GetIO().WantCaptureMouse));
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

    constexpr float kRepeatDelay    = 0.4f;
    constexpr float kRepeatInterval = 0.05f;

    if (!blockKeyboardInput && Core::getPlatform().getKey(GLFW_KEY_LEFT)) {
        AnimationHandler& anim = Core::getAnimation();
        if (!prevKeys[GLFW_KEY_LEFT]) {
            anim.reset(std::max(0, anim.getFrame() - 1));
            anim.pause();
            Core::requestAccumulationRestart();
            leftRepeat = kRepeatDelay;
        } else {
            leftRepeat -= dt;
            if (leftRepeat <= 0.0f) {
                anim.reset(std::max(0, anim.getFrame() - 1));
                anim.pause();
                Core::requestAccumulationRestart();
                leftRepeat += kRepeatInterval;
            }
        }
        prevKeys[GLFW_KEY_LEFT] = true;
    } else {
        prevKeys[GLFW_KEY_LEFT] = false;
        leftRepeat = 0.0f;
    }

    if (!blockKeyboardInput && Core::getPlatform().getKey(GLFW_KEY_RIGHT)) {
        AnimationHandler& anim = Core::getAnimation();
        if (!prevKeys[GLFW_KEY_RIGHT]) {
            anim.reset(std::min(anim.getEndFrame() - 1, anim.getFrame() + 1));
            anim.pause();
            Core::requestAccumulationRestart();
            rightRepeat = kRepeatDelay;
        } else {
            rightRepeat -= dt;
            if (rightRepeat <= 0.0f) {
                anim.reset(std::min(anim.getEndFrame() - 1, anim.getFrame() + 1));
                anim.pause();
                Core::requestAccumulationRestart();
                rightRepeat += kRepeatInterval;
            }
        }
        prevKeys[GLFW_KEY_RIGHT] = true;
    } else {
        prevKeys[GLFW_KEY_RIGHT] = false;
        rightRepeat = 0.0f;
    }

    if (!blockKeyboardInput) {
        if (Core::getScene().getCamera().processInput(dt)) {
            Core::requestAccumulationRestart();
            ecs::syncPreviewCameraToEntity(Core::getScene().getCamera(), Core::getScene().getRegistry());
        }
    }

    if (!blockKeyboardInput && Core::getPlatform().getKey(GLFW_KEY_R))
        Core::requestAccumulationRestart();

    if (Core::getScene().checkUpdate())
        Core::requestAccumulationRestart();
}

bool InputHandler::justPressed(int key) {
    const bool down = Core::getPlatform().getKey(key);
    const bool first = down && !prevKeys[key];
    prevKeys[key] = down;
    return first;
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
