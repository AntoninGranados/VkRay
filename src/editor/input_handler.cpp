#include "input_handler.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "imgui/imgui.h"
#include "imgui/ImGuizmo.h"

#include "VkSmol/platform/platform.hpp"

#include "core/animation_handler.hpp"
#include "app_context.hpp"
#include "application.hpp"
#include "core/camera.hpp"
#include "editor/editor_ui.hpp"
#include "core/scene/scene.hpp"


void InputHandler::initCallbacks(const AppContext& ctx) {
    ctx.platform->setCursorPosCallback([&ctx](double x, double y){
        const bool cameraLocked = ctx.camera->isLocked();
        if (cameraLocked && (ImGui::GetIO().WantCaptureMouse || ctx.ui->isMouseCaptured() || ImGuizmo::IsUsing()))
            return;
        *ctx.restartRender |= ctx.camera->cursorPosCallback(x, y);
    });

    ctx.platform->setScrollCallback([&ctx](double xoffset, double yoffset){
        if (ImGui::GetIO().WantCaptureMouse || ctx.ui->isMouseCaptured()) return;
        if (ctx.renderMode != RenderMode::Preview) return;
        *ctx.restartRender |= ctx.camera->scrollCallback(xoffset, yoffset);
    });
}

void InputHandler::pollEvents(const AppContext& ctx) {
    ctx.platform->pollEvents();
}

void InputHandler::handle(AppContext& ctx, float dt) {
    switch (ctx.renderMode) {
        case RenderMode::Preview:           handlePreview(ctx, dt); break;
        case RenderMode::RenderSingle:
        case RenderMode::RenderAnimation:   handleRender(ctx, dt);  break;
    }
}

void InputHandler::handlePreview(AppContext& ctx, float dt) {
    const bool blockMouseInput = ImGuizmo::IsUsing() || (ctx.camera->isLocked() && (ctx.ui->isMouseCaptured() || ImGui::GetIO().WantCaptureMouse));
    const bool blockKeyboardInput = ctx.ui->isKeyboardCaptured() || ImGui::GetIO().WantCaptureKeyboard;

    ctx.platform->setCursorMode(
        (ctx.camera->isLocked() || blockMouseInput) ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED
    );

    const bool middleDown = ctx.platform->getMouseButton(GLFW_MOUSE_BUTTON_MIDDLE);
    if (!blockMouseInput && middleDown && !middleClickWasDown) {
        double xpos, ypos;
        ctx.platform->getCursorPos(xpos, ypos);
        int width, height;
        ctx.platform->getWindowSize(width, height);
        float dist;
        if (ctx.ui->focusDepthAt(*ctx.scene, { xpos, ypos }, { static_cast<float>(width), static_cast<float>(height) }, dist)) {
            ctx.camera->setFocusDepth(dist);
            *ctx.restartRender = true;
        }
    }
    middleClickWasDown = middleDown;

    if (!blockMouseInput && ctx.platform->getMouseButton(GLFW_MOUSE_BUTTON_LEFT)) {
        double xpos, ypos;
        ctx.platform->getCursorPos(xpos, ypos);
        int width, height;
        ctx.platform->getWindowSize(width, height);
        ctx.ui->pickEntity(*ctx.scene, { xpos, ypos }, { static_cast<float>(width), static_cast<float>(height) });
    }

    if (ctx.platform->getKey(GLFW_KEY_ESCAPE)) {
        if (!ctx.ui->isToggled()) ctx.ui->toggle();
        else ctx.ui->clearEntitySelection();
    }

    if (!blockKeyboardInput && ctx.camera->isLocked() && ctx.platform->getKey(GLFW_KEY_SPACE)) {
        if (!spaceWasDown) ctx.animation->toggle();
        spaceWasDown = true;
    } else {
        spaceWasDown = false;
    }

    if (!blockKeyboardInput) {
        *ctx.restartRender |= ctx.camera->processInput(*ctx.platform, dt);
    }

    if (!blockKeyboardInput && ctx.platform->getKey(GLFW_KEY_R)) {
        *ctx.restartRender = true;
    }

    if (ctx.scene->checkUpdate()) {
        *ctx.restartRender = true;
    }
}

void InputHandler::handleRender(AppContext& ctx, float dt) {
    ctx.platform->setCursorMode(GLFW_CURSOR_NORMAL);
    if (ctx.platform->getKey(GLFW_KEY_ESCAPE)) {
        returnToPreview(ctx);
    }
}

void InputHandler::returnToPreview(AppContext& ctx) {
    ctx.ui->restoreToggledState();
    ctx.renderMode = RenderMode::Preview;
}
