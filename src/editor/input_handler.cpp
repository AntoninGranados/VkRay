#include "input_handler.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "imgui/imgui.h"
#include "imgui/ImGuizmo.h"

#include "VkSmol/platform/platform.hpp"

#include "core/animation_handler.hpp"
#include "core/camera.hpp"
#include "editor/editor.hpp"
#include "editor/editor_ui.hpp"
#include "core/core.hpp"
#include "core/scene/scene.hpp"


void InputHandler::initCallbacks() {
    Core::getPlatform().setCursorPosCallback([](double x, double y){
        const bool cameraLocked = Core::getScene().getCamera().isLocked();
        if (cameraLocked && (ImGui::GetIO().WantCaptureMouse || Editor::getUi().isMouseCaptured() || ImGuizmo::IsUsing()))
            return;
        if (Core::getScene().getCamera().cursorPosCallback(x, y)) Core::restartAccumulation();
    });

    Core::getPlatform().setScrollCallback([](double xoffset, double yoffset){
        if (ImGui::GetIO().WantCaptureMouse || Editor::getUi().isMouseCaptured()) return;
        if (Core::getRenderMode() != RenderMode::Preview) return;
        if (Core::getScene().getCamera().scrollCallback(xoffset, yoffset)) Core::restartAccumulation();
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
    const bool blockMouseInput = ImGuizmo::IsUsing() || (Core::getScene().getCamera().isLocked() && (Editor::getUi().isMouseCaptured() || ImGui::GetIO().WantCaptureMouse));
    const bool blockKeyboardInput = Editor::getUi().isKeyboardCaptured() || ImGui::GetIO().WantCaptureKeyboard;

    Core::getPlatform().setCursorMode(
        (Core::getScene().getCamera().isLocked() || blockMouseInput) ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED
    );

    const bool middleDown = Core::getPlatform().getMouseButton(GLFW_MOUSE_BUTTON_MIDDLE);
    if (!blockMouseInput && middleDown && !middleClickWasDown) {
        double xpos, ypos;
        Core::getPlatform().getCursorPos(xpos, ypos);
        int width, height;
        Core::getPlatform().getWindowSize(width, height);
        float dist;
        if (Editor::getUi().focusDepthAt(Core::getScene(), { xpos, ypos }, { static_cast<float>(width), static_cast<float>(height) }, dist)) {
            Core::getScene().getCamera().setFocusDepth(dist);
            Core::restartAccumulation();
        }
    }
    middleClickWasDown = middleDown;

    if (!blockMouseInput && Core::getPlatform().getMouseButton(GLFW_MOUSE_BUTTON_LEFT)) {
        double xpos, ypos;
        Core::getPlatform().getCursorPos(xpos, ypos);
        int width, height;
        Core::getPlatform().getWindowSize(width, height);
        Editor::getUi().pickEntity(Core::getScene(), { xpos, ypos }, { static_cast<float>(width), static_cast<float>(height) });
    }

    if (Core::getPlatform().getKey(GLFW_KEY_ESCAPE)) {
        if (!Editor::getUi().isToggled()) Editor::getUi().toggle();
        else Editor::getUi().clearEntitySelection();
    }

    if (!blockKeyboardInput && Core::getScene().getCamera().isLocked() && Core::getPlatform().getKey(GLFW_KEY_SPACE)) {
        if (!spaceWasDown) Core::getAnimation().toggle();
        spaceWasDown = true;
    } else {
        spaceWasDown = false;
    }

    if (!blockKeyboardInput) {
        if (Core::getScene().getCamera().processInput(dt)) Core::restartAccumulation();
    }

    if (!blockKeyboardInput && Core::getPlatform().getKey(GLFW_KEY_R)) {
        Core::restartAccumulation();
    }

    if (Core::getScene().checkUpdate()) {
        Core::restartAccumulation();
    }
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
