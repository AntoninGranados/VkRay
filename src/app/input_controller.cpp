#include "input_controller.hpp"

#include <algorithm>
#include <cmath>

#include "imgui/imgui.h"
#include "imgui/ImGuizmo.h"

#include "application.hpp"
#include "app_context.hpp"
#include "camera.hpp"
#include "ui_handler.hpp"
#include "parameter_handler.hpp"
#include "animation_handler.hpp"
#include "scene/scene.hpp"
#include "engine/engine.hpp"


void InputController::initCallbacks(const AppContext& ctx) {
    glfwSetCursorPosCallback(
        ctx.engine->getWindow().get(),
        [](GLFWwindow* window, double x, double y) {
            ImGui_ImplGlfw_CursorPosCallback(window, x, y);
            auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
            const bool cameraLocked = app->ctx.camera->isLocked();
            if (cameraLocked && (ImGui::GetIO().WantCaptureMouse || app->ui.isMouseCaptured() || ImGuizmo::IsUsing()))
                return;
            app->restartRender |= app->ctx.camera->cursorPosCallback(window, x, y);
        }
    );

    glfwSetScrollCallback(
        ctx.engine->getWindow().get(), 
        [](GLFWwindow* window, double xoffset, double yoffset) {
            ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
            auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
            if (ImGui::GetIO().WantCaptureMouse || app->ui.isMouseCaptured()) return;
            if (app->ctx.renderState->renderMode != RenderMode::Preview) return;
            app->restartRender |= app->ctx.camera->scrollCallback(window, xoffset, yoffset);
        }
    );
}

void InputController::pollEvents() {
    glfwPollEvents();
}

void InputController::handle(const AppContext& ctx, float dt) {
    switch (ctx.renderState->renderMode) {
        case RenderMode::Preview:           handlePreview(ctx, dt); break;
        case RenderMode::RenderSingle:
        case RenderMode::RenderAnimation:   handleRender(ctx, dt);  break;
    }
}

void InputController::handlePreview(const AppContext& ctx, float dt) {
    GLFWwindow* window = ctx.engine->getWindow().get();
    ctx.renderState->resolution = ctx.parameters->getFloat("previewResolution");

    const bool blockMouseInput = ImGuizmo::IsUsing() || (ctx.camera->isLocked() && (ctx.ui->isMouseCaptured() || ImGui::GetIO().WantCaptureMouse));
    const bool blockKeyboardInput = ctx.ui->isKeyboardCaptured() || ImGui::GetIO().WantCaptureKeyboard;

    // Set cursor mode: hiddent when the camera is not locked or the mouse is blocked
    glfwSetInputMode(
        window,
        GLFW_CURSOR,
        (ctx.camera->isLocked() || blockMouseInput) ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED
    );

    // Middle-click performs a focus-depth pick
    const bool middleDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
    if (!blockMouseInput && middleDown && !middleClickWasDown) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        float dist;
        glm::vec3 p;
        if (ctx.scene->raycast({ xpos, ypos }, { static_cast<float>(width), static_cast<float>(height) }, dist, p, false, false)) {
            ctx.camera->setFocusDepth(dist);
            *ctx.restartRender = true;
        }
    }
    middleClickWasDown = middleDown;

    // Left-click pick the targeted object
    if (!blockMouseInput && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        float dist;
        glm::vec3 p;
        ctx.scene->raycast({ xpos, ypos }, { static_cast<float>(width), static_cast<float>(height) }, dist, p, true);
    }

    // On escape: reopen ui if it is hidden; unselect object
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        if (!ctx.ui->isToggled()) ctx.ui->toggle();
        else ctx.scene->clearSelection();
    }

    // Space toggles play/pause only on the rising edge to avoid repeat toggles while held.
    if (!blockKeyboardInput && ctx.camera->isLocked() && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        if (!spaceWasDown) ctx.animation->toggle();
        spaceWasDown = true;
    } else {
        spaceWasDown = false;
    }

    // Process camera input
    if (!blockKeyboardInput) {
        *ctx.restartRender |= ctx.camera->processInput(window, dt);
    }

    // Restart rendering when `R` is pressed
    if (!blockKeyboardInput && glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        *ctx.restartRender = true;
    }
    
    if (ctx.scene->checkUpdate()) {
        *ctx.restartRender = true;
    }
}

void InputController::handleRender(const AppContext& ctx, float dt) {
    GLFWwindow* window = ctx.engine->getWindow().get();
    ctx.renderState->resolution = ctx.parameters->getFloat("renderResolution");
    updateRenderSamplesPerSecond(ctx, dt);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        returnToPreview(ctx);
    }
}

void InputController::updateRenderSamplesPerSecond(const AppContext& ctx, float dt) {
    const double dtSafe = std::max(static_cast<double>(dt), 0.0);
    ctx.renderState->samplesPerSecAccumTime += dtSafe;
    ctx.renderState->samplesPerSecAccumSamples += static_cast<double>(ctx.parameters->getInt("previewSamples"));

    if (ctx.renderState->samplesPerSecAccumTime >= 1.0) {
        // Smooth instantaneous throughput with an EMA to reduce noisy ETA/UI updates.
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

void InputController::returnToPreview(const AppContext& ctx) {
    ctx.ui->restorToggledState();
    ctx.renderState->renderMode = RenderMode::Preview;
    ctx.renderState->pendingExit = false;
    ctx.renderState->samplesPerSecEMA = 0.0;
    ctx.renderState->samplesPerSecInitialized = false;
    ctx.renderState->samplesPerSecAccumTime = 0.0;
    ctx.renderState->samplesPerSecAccumSamples = 0.0;
}
