#include "camera_control_system.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include "imgui/imgui.h"

#include "VkSmol/platform/platform.hpp"

#include "core/camera/camera.hpp"
#include "core/core.hpp"
#include "core/ecs/components/camera.hpp"
#include "core/ecs/components/component.hpp"
#include "core/ecs/components/core.hpp"
#include "core/ecs/entity.hpp"
#include "core/parameters/parameters.hpp"
#include "core/scene/scene.hpp"
#include "editor/ecs/components/camera.hpp"

namespace ecs {

void cameraActivationSystem(Registry& registry) {
    const ecs::Entity camera = Core::getScene().getCamera();
    if (!registry.has(camera, CameraNavigation)) registry.add(camera, CameraNavigation);
}

void cameraControlSystem(Registry& registry) {
    if (Core::getRenderMode() != RenderMode::Preview) return;    // don't move the cameras when rendering

    const ecs::Entity camera = Core::getScene().getCamera();

    Platform& platform = Core::getPlatform();
    ecs::Component& navigation = registry.get(camera, ecs::CameraNavigation);
    CameraNavigationState& state = navigation.payload<CameraNavigationState>("state");
    ecs::Component& t = registry.get(camera, ecs::Transform);

    const bool rmb   = platform.getMouseButton(GLFW_MOUSE_BUTTON_RIGHT);
    const bool mmb   = platform.getMouseButton(GLFW_MOUSE_BUTTON_MIDDLE);
    const bool shift = platform.getKey(GLFW_KEY_LEFT_SHIFT) || platform.getKey(GLFW_KEY_RIGHT_SHIFT);
    const bool ctrl  = platform.getKey(GLFW_KEY_LEFT_CONTROL) || platform.getKey(GLFW_KEY_RIGHT_CONTROL);

    DragMode newDragMode = DragMode::None;
    if (rmb) {
        newDragMode = DragMode::Look;
    } else if (mmb) {
        if (shift) newDragMode = DragMode::Pan;
        else if (ctrl) newDragMode = DragMode::Dolly;
        else newDragMode = DragMode::Orbit;
    }

    if (newDragMode != state.dragMode) {
        state.dragMode = newDragMode;
        if (state.dragMode != DragMode::None) {
            state.firstMouse = true;
            state.orbitDistance = glm::max(0.1f, glm::length(state.anchor - t.get<glm::vec3>("position")));
        }
    }

    state.locked = (state.dragMode == DragMode::None);

    if (state.dragMode == DragMode::Look) {
        const glm::vec3 dir = directionFromRotation(t.get<glm::vec3>("rotation"));
        const glm::vec3 right = glm::normalize(glm::cross(dir, glm::vec3(0.0f, 1.0f, 0.0f)));

        const float speed = Core::getParameters().get<float>("editor/camera/speed");
        float velocity = speed * ImGui::GetIO().DeltaTime;
        if (platform.getKey(GLFW_KEY_LEFT_CONTROL)) velocity /= 8.0f;

        glm::vec3 position = t.get<glm::vec3>("position");
        bool moved = false;

        if (platform.getKey(GLFW_KEY_W))          { position += dir * velocity;              moved = true; }
        if (platform.getKey(GLFW_KEY_S))          { position -= dir * velocity;              moved = true; }
        if (platform.getKey(GLFW_KEY_A))          { position -= right * velocity;            moved = true; }
        if (platform.getKey(GLFW_KEY_D))          { position += right * velocity;            moved = true; }
        if (platform.getKey(GLFW_KEY_SPACE))      { position += glm::vec3(0, 1, 0) * velocity; moved = true; }
        if (platform.getKey(GLFW_KEY_LEFT_SHIFT)) { position -= glm::vec3(0, 1, 0) * velocity; moved = true; }

        if (moved) {
            state.anchor += position - t.get<glm::vec3>("position");
            t.set<glm::vec3>("position", position);
            Core::markRenderDirty();
        }
    }
}

void cameraCursorCallback(Registry& registry, ecs::Entity camera, double x, double y) {
    const ecs::Component& c = registry.get(camera, ecs::Camera);
    ecs::Component& navigation = registry.get(camera, ecs::CameraNavigation);
    CameraNavigationState& state = navigation.payload<CameraNavigationState>("state");

    if (state.locked || state.dragMode == DragMode::None) return;

    ecs::Component& t = registry.get(camera, ecs::Transform);
    const glm::vec3 rotation = t.get<glm::vec3>("rotation");
    const glm::vec3 dir = directionFromRotation(rotation);
    const glm::vec3 right = glm::normalize(glm::cross(dir, glm::vec3(0, 1, 0)));
    const glm::vec3 camUp = glm::normalize(glm::cross(right, dir));

    if (state.firstMouse) {
        state.lastX = x;
        state.lastY = y;
        state.firstMouse = false;
    }

    float xoffset = x - state.lastX;
    float yoffset = state.lastY - y;
    if (xoffset != 0 || yoffset != 0) Core::markRenderDirty();
    state.lastX = x;
    state.lastY = y;

    if (state.dragMode == DragMode::Pan) {
        const float panSensitivity = Core::getParameters().get<float>("editor/camera/pan_sensitivity");
        float panScale = panSensitivity * state.orbitDistance;
        glm::vec3 offset = (right * xoffset + camUp * yoffset) * panScale;
        t.set<glm::vec3>("position", t.get<glm::vec3>("position") - offset);
        state.anchor -= offset;
        return;
    }

    if (state.dragMode == DragMode::Dolly) {
        const float dollySensitivity = Core::getParameters().get<float>("editor/camera/dolly_sensitivity");
        float dollyDelta = yoffset * dollySensitivity * state.orbitDistance;
        state.orbitDistance = glm::max(0.1f, state.orbitDistance + dollyDelta);
        t.set<glm::vec3>("position", state.anchor - dir * state.orbitDistance);
        return;
    }

    const float sensitivity = Core::getParameters().get<float>("editor/camera/sensitivity");
    float zoomSensitivityFactor = glm::min(c.get<float>("fov") / 80.0f, 1.0f);
    xoffset *= sensitivity * zoomSensitivityFactor;
    yoffset *= sensitivity * zoomSensitivityFactor;

    glm::vec3 newRotation = rotation;
    newRotation.y -= xoffset;
    newRotation.x = glm::clamp(newRotation.x + yoffset, -89.0f, 89.0f);
    t.set<glm::vec3>("rotation", newRotation);

    const glm::vec3 newDir = directionFromRotation(newRotation);

    if (state.dragMode == DragMode::Look) {
        state.anchor = t.get<glm::vec3>("position") + newDir * state.orbitDistance;
        return;
    }

    if (state.dragMode == DragMode::Orbit) {
        t.set<glm::vec3>("position", state.anchor - newDir * state.orbitDistance);
        return;
    }
}

void cameraScrollCallback(Registry& registry, ecs::Entity camera, double xoffset, double yoffset) {
    if (Core::getRenderMode() != RenderMode::Preview) return;

    auto& c = registry.get(camera, ecs::Camera);
    c.set<float>("fov", c.get<float>("fov") - static_cast<float>(yoffset));
    if (yoffset != 0) Core::markRenderDirty();
}

} // namespace ecs
