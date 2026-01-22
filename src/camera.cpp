#include "camera.hpp"

#include <cmath>

void updateYawPitchFromDirection(const glm::vec3 &dir, float &yaw, float &pitch) {
    glm::vec3 n = glm::normalize(dir);
    yaw = glm::degrees(atan2(n.z, n.x));
    pitch = glm::degrees(asin(n.y));
}

Camera::Camera(glm::vec3 position)
    : CameraHandle("Preview Camera", position, glm::vec3(0.0f, 0.0f, 1.0f), 80.0f),
      target(glm::vec3(0.0f)) {
    setDirection(target - position);
    orbitDistance = glm::length(target - position);
    if (orbitDistance < 0.1f) orbitDistance = 0.1f;
    updateYawPitchFromDirection(getDirection(), yaw, pitch);
}

bool Camera::cursorPosCallback(GLFWwindow *window, double x, double y) {
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
        return change;
    }

    if (dragMode == DragMode::Dolly) {
        float dollyDelta = yoffset * dollySensitivity * orbitDistance;
        orbitDistance = glm::max(0.1f, orbitDistance + dollyDelta);
        position = target - dir * orbitDistance;
        setTarget(target);
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
        return change;
    }

    if (dragMode == DragMode::Orbit) {
        position = target - newDir * orbitDistance;
        setTarget(target);
        return change;
    }

    return change;
}

bool Camera::scrollCallback(GLFWwindow *window, double xoffset, double yoffset) {
    float newFov = getFov() - static_cast<float>(yoffset);
    if (newFov < 1.0f) newFov = 1.0f;
    if (newFov > 160.0f) newFov = 160.0f;
    setFov(newFov);
    return yoffset != 0;
}

bool Camera::processInput(GLFWwindow *window, float deltaTime) {
    float velocity = speed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        velocity /= 8.0f;
    }

    bool change = false;

    const bool rmb = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    const bool mmb = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
    const bool shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
    const bool ctrl = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;

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

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            position += dir * velocity;
            target += dir * velocity;
            change = true;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            position -= dir * velocity;
            target -= dir * velocity;
            change = true;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            position -= right * velocity;
            target -= right * velocity;
            change = true;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            position += right * velocity;
            target += right * velocity;
            change = true;
        }
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            position += getUp() * velocity;
            target += getUp() * velocity;
            change = true;
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            position -= getUp() * velocity;
            target -= getUp() * velocity;
            change = true;
        }
    }

    if (change)
        setTarget(target);

    return change;
}

bool Camera::drawPreviewUI(bool &restartRequested) {
    bool updated = false;

    glm::vec3 dir = getDirection();
    ImGui::Text("Camera Position :\n (%4.1f, %4.1f, %4.1f)", position.x, position.y, position.z);
    ImGui::Text("Camera Direction:\n (%4.1f, %4.1f, %4.1f)", dir.x, dir.y, dir.z);
    ImGui::Text("Camera Target   :\n (%4.1f, %4.1f, %4.1f)", target.x, target.y, target.z);
    ImGui::Text("Camera Fov:\n %4.1f°", getFov());
    
    ImGui::Text("Camera Aperture:");
    ImGui::SetNextItemWidth(-FLT_MIN);
    float newAperture = getAperture();
    if (ImGui::DragFloat("##Camera Aperture", &newAperture, 0.01, 0.0, 5.0)) {
        setAperture(newAperture);
        updated = true;
    }

    ImGui::Text("Camera Focus Depth:");
    ImGui::SetNextItemWidth(-FLT_MIN);
    float newFocusDepth = getFocusDepth();
    if (ImGui::DragFloat("##Camera Focus Depth", &newFocusDepth, 0.1, 0.0, 100.0)) {
        setFocusDepth(newFocusDepth);
        updated = true;
    }

    // This is redundant (same value as the one returned) but it makes it consistent with the parameters update
    if (updated)
        restartRequested = true;
    return updated;
}
