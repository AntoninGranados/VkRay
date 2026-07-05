#include "input_handler.hpp"

#include <algorithm>
#include <cmath>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "imgui/imgui.h"
#include "imgui/ImGuizmo.h"

#include "VkSmol/engine.hpp"
#include "VkSmol/platform/platform.hpp"

#include "app/animation_handler.hpp"
#include "app/app_context.hpp"
#include "app/parameter_handler.hpp"
#include "application.hpp"
#include "camera.hpp"
#include "editor/editor_ui.hpp"
#include "scene/scene.hpp"


void InputHandler::initCallbacks(const AppContext& ctx) {
    ctx.platform->setCursorPosCallback([&ctx](double x, double y){
        const bool cameraLocked = ctx.camera->isLocked();
        if (cameraLocked && (ImGui::GetIO().WantCaptureMouse || ctx.ui->isMouseCaptured() || ImGuizmo::IsUsing()))
            return;
        *ctx.restartRender |= ctx.camera->cursorPosCallback(x, y);
    });

    ctx.platform->setScrollCallback([&ctx](double xoffset, double yoffset){
        if (ImGui::GetIO().WantCaptureMouse || ctx.ui->isMouseCaptured()) return;
        if (ctx.renderState->renderMode != RenderMode::Preview) return;
        *ctx.restartRender |= ctx.camera->scrollCallback(xoffset, yoffset);
    });
}

void InputHandler::pollEvents(const AppContext& ctx) {
    ctx.platform->pollEvents();
}

void InputHandler::handle(const AppContext& ctx, float dt) {
    switch (ctx.renderState->renderMode) {
        case RenderMode::Preview:           handlePreview(ctx, dt); break;
        case RenderMode::RenderSingle:
        case RenderMode::RenderAnimation:   handleRender(ctx, dt);  break;
    }
}

void InputHandler::handlePreview(const AppContext& ctx, float dt) {
    ctx.renderState->resolution = ctx.parameters->getFloat("pathtracer/resolution/preview");

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
        glm::vec3 p;
        if (ctx.scene->raycast({ xpos, ypos }, { static_cast<float>(width), static_cast<float>(height) }, dist, p, false, false)) {
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
        float dist;
        glm::vec3 p;
        ctx.scene->raycast({ xpos, ypos }, { static_cast<float>(width), static_cast<float>(height) }, dist, p, true);
    }

    if (ctx.platform->getKey(GLFW_KEY_ESCAPE)) {
        if (!ctx.ui->isToggled()) ctx.ui->toggle();
        else ctx.scene->clearSelection();
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

void InputHandler::handleRender(const AppContext& ctx, float dt) {
    ctx.renderState->resolution = ctx.parameters->getFloat("pathtracer/resolution/render");
    updateRenderSamplesPerSecond(ctx, dt);

    ctx.platform->setCursorMode(GLFW_CURSOR_NORMAL);
    if (ctx.platform->getKey(GLFW_KEY_ESCAPE)) {
        returnToPreview(ctx);
    }
}

void InputHandler::updateRenderSamplesPerSecond(const AppContext& ctx, float dt) {
    const double dtSafe = std::max(static_cast<double>(dt), 0.0);
    ctx.renderState->samplesPerSecAccumTime += dtSafe;
    ctx.renderState->samplesPerSecAccumSamples += static_cast<double>(ctx.parameters->getInt("pathtracer/sampling/preview_samples"));

    if (ctx.renderState->samplesPerSecAccumTime >= 1.0) {
        const double instant = ctx.renderState->samplesPerSecAccumSamples / std::max(ctx.renderState->samplesPerSecAccumTime, 1e-6);
        const double alpha = 1.0 - std::exp(-ctx.renderState->samplesPerSecAccumTime / 5.0);
        if (!ctx.renderState->samplesPerSecInitialized) {
            ctx.renderState->samplesPerSecEMA = instant;
            ctx.renderState->samplesPerSecInitialized = true;
        } else {
            ctx.renderState->samplesPerSecEMA += alpha * (instant - ctx.renderState->samplesPerSecEMA);
        }
        ctx.renderState->samplesPerSecAccumTime = 0.0;
        ctx.renderState->samplesPerSecAccumSamples = 0.0;
    }
}

void InputHandler::returnToPreview(const AppContext& ctx) {
    ctx.ui->restoreToggledState();
    ctx.renderState->renderMode = RenderMode::Preview;
    ctx.renderState->pendingExit = false;
    ctx.renderState->samplesPerSecEMA = 0.0;
    ctx.renderState->samplesPerSecInitialized = false;
    ctx.renderState->samplesPerSecAccumTime = 0.0;
    ctx.renderState->samplesPerSecAccumSamples = 0.0;
}
