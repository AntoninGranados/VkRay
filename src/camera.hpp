#pragma once

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "imgui/imgui.h"
#include "scene/camera_handle.hpp"

class Camera : public CameraHandle {
public:
    Camera(glm::vec3 position = glm::vec3(0.0f));

    bool cursorPosCallback(GLFWwindow *window, double x, double y);
    bool scrollCallback(GLFWwindow *window, double xoffset, double yoffset);
    bool processInput(GLFWwindow *window, float deltaTime);
    
    float getTanHFov() const { return glm::tan(glm::radians(getFov()) * 0.5f); }
    glm::vec3 getDirection() const { return glm::normalize(target - position); };
    glm::mat4 getView() const { return glm::lookAt(position, target, getUp()); };

    glm::vec3 getTarget() const { return target; }
    void setTarget(glm::vec3 newTarget) { target = newTarget; setDirection(target - position); }

    bool isLocked() { return locked; }
    void toggleLock() { locked = !locked; }

    void resetMouse() { firstMouse = true; }

    bool drawPreviewUI(bool &restartRequested);

private:
    enum class DragMode {
        None,
        Look,
        Orbit,
        Pan,
        Dolly
    };

    float orbitDistance = 10.0f;

    glm::vec3 target;
    float yaw   = 90.0f;
    float pitch = 0.0f;

    float lastX = 0.0f;
    float lastY = 0.0f;
    bool firstMouse = true;

    float speed = 20.0f;
    float sensitivity = 0.2f;
    float panSensitivity = 0.003f;
    float dollySensitivity = 0.01f;

    DragMode dragMode = DragMode::None;
    bool locked = true;
};
