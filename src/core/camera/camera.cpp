#include "camera.hpp"

#include <cmath>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "core/core.hpp"
#include "core/ecs/components.hpp"

namespace {

void updateYawPitchFromDirection(const glm::vec3& dir, float& yaw, float& pitch) {
    const glm::vec3 n = glm::normalize(dir);
    yaw = glm::degrees(atan2(n.z, n.x));
    pitch = glm::degrees(asin(n.y));
}

} // namespace

void Camera::syncToPreviewTransform() {
    ecs::Registry& reg = Core::getScene().getRegistry();
    if (!reg.has(*previewEntity, ecs::Transform)) return;
    ecs::Component& t = reg.get(*previewEntity, ecs::Transform);
    const glm::quat q = glm::quatLookAt(glm::normalize(getDirection()), getUp());
    t.set<glm::vec3>("position", position);
    t.set<glm::vec3>("rotation", glm::degrees(glm::eulerAngles(q)));
}

// Public
Camera::Camera(glm::vec3 position)
    : position(position),
      target(glm::vec3(0.0f)) {
    orbitDistance = glm::length(target - position);
    if (orbitDistance < 0.1f) orbitDistance = 0.1f;
    updateYawPitchFromDirection(getDirection(), yaw, pitch);
}

bool Camera::cursorPosCallback(double x, double y) {
    if (locked || dragMode == DragMode::None) return false;

    bool change = false;

    if (firstMouse) {
        lastX = x;
        lastY = y;
        firstMouse = false;
        updateYawPitchFromDirection(getDirection(), yaw, pitch);
    }

    float xoffset = x - lastX;
    float yoffset = lastY - y;
    if (xoffset != 0 || yoffset != 0) change = true;
    lastX = x;
    lastY = y;

    const glm::vec3 dir = getDirection();
    const glm::vec3 right = glm::normalize(glm::cross(dir, getUp()));
    const glm::vec3 camUp = glm::normalize(glm::cross(right, dir));

    if (dragMode == DragMode::Pan) {
        float panScale = panSensitivity * orbitDistance;
        glm::vec3 offset = (right * xoffset + camUp * yoffset) * panScale;
        position -= offset;
        target -= offset;
        setTarget(target);
        if (change && previewEntity) syncToPreviewTransform();
        return change;
    }

    if (dragMode == DragMode::Dolly) {
        float dollyDelta = yoffset * dollySensitivity * orbitDistance;
        orbitDistance = glm::max(0.1f, orbitDistance + dollyDelta);
        position = target - dir * orbitDistance;
        setTarget(target);
        if (change && previewEntity) syncToPreviewTransform();
        return change;
    }

    float zoomSensitivityFactor = glm::min(getFov() / 80.0f, 1.0f);
    xoffset *= sensitivity * zoomSensitivityFactor;
    yoffset *= sensitivity * zoomSensitivityFactor;

    yaw   += xoffset;
    pitch = glm::clamp(pitch + yoffset, -89.0f, 89.0f);

    glm::vec3 newDir;
    newDir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newDir.y = sin(glm::radians(pitch));
    newDir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    newDir = glm::normalize(newDir);

    if (dragMode == DragMode::Look) {
        target = position + newDir * orbitDistance;
        setTarget(target);
        if (change && previewEntity) syncToPreviewTransform();
        return change;
    }

    if (dragMode == DragMode::Orbit) {
        position = target - newDir * orbitDistance;
        setTarget(target);
        if (change && previewEntity) syncToPreviewTransform();
        return change;
    }

    return change;
}

bool Camera::scrollCallback(double xoffset, double yoffset) {
    setFov(getFov() - static_cast<float>(yoffset));
    return yoffset != 0;
}

bool Camera::processInput(float deltaTime) {
    Platform& platform = Core::getPlatform();
    float velocity = speed * deltaTime;
    if (platform.getKey(GLFW_KEY_LEFT_CONTROL)) {
        velocity /= 8.0f;
    }

    bool change = false;

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

    if (newDragMode != dragMode) {
        dragMode = newDragMode;
        if (dragMode != DragMode::None) {
            resetMouse();
            orbitDistance = glm::length(target - position);
            if (orbitDistance < 0.1f) orbitDistance = 0.1f;
        }
    }

    locked = (dragMode == DragMode::None);

    if (dragMode == DragMode::Look) {
        glm::vec3 dir = getDirection();
        glm::vec3 right = glm::normalize(glm::cross(dir, getUp()));

        if (platform.getKey(GLFW_KEY_W)) {
            position += dir * velocity;
            target += dir * velocity;
            change = true;
        }
        if (platform.getKey(GLFW_KEY_S)) {
            position -= dir * velocity;
            target -= dir * velocity;
            change = true;
        }
        if (platform.getKey(GLFW_KEY_A)) {
            position -= right * velocity;
            target -= right * velocity;
            change = true;
        }
        if (platform.getKey(GLFW_KEY_D)) {
            position += right * velocity;
            target += right * velocity;
            change = true;
        }
        if (platform.getKey(GLFW_KEY_SPACE)) {
            position += getUp() * velocity;
            target += getUp() * velocity;
            change = true;
        }
        if (platform.getKey(GLFW_KEY_LEFT_SHIFT)) {
            position -= getUp() * velocity;
            target -= getUp() * velocity;
            change = true;
        }
    }

    if (change) {
        setTarget(target);
        if (previewEntity) syncToPreviewTransform();
    }

    return change;
}

glm::mat4 Camera::getProjection(float aspect) const {
    return glm::perspective(glm::radians(fov), aspect, 1e-4f, 1e4f);
}
