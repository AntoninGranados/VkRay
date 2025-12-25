#pragma once

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "imgui/imgui.h"

class Camera {
public:
    Camera(glm::vec3 position);

    bool cursorPosCallback(GLFWwindow *window, double x, double y);
    bool scrollCallback(GLFWwindow *window, double xoffset, double yoffset);
    bool processInput(GLFWwindow *window, float deltaTime);
    
    float getTanHFov() const { return glm::tan(glm::radians(fov) * 0.5); }
    float getFov() const { return fov; }
    glm::vec3 getPosition() const { return position; }
    glm::vec3 getDirection() const;
    glm::vec3 getUp() const { return up; }
    glm::mat4 getView() const;
    glm::mat4 getProjection(GLFWwindow* window) const;

    void setTarget(glm::vec3 newTarget) { target = newTarget; }
    float getAperture() const { return aperture; };
    void setAperture(float newAperture) { aperture = newAperture; }
    float getFocusDepth() const { return focusDepth; };
    void setFocusDepth(float newFocusDepth) { focusDepth = newFocusDepth; }

    bool isLocked() { return locked; }
    void toggleLock() { locked = !locked; }

    void resetMouse() { firstMouse = true; }

    bool drawUI();

private:
    enum class DragMode {
        None,
        Look,
        Orbit,
        Pan,
        Dolly
    };

    float fov = 80.0f;

    float aperture = 0.0f;
    float focusDepth = 10.0f;

    glm::vec3 position;
    glm::vec3 target;
    glm::vec3 up = { 0.0, 1.0, 0.0 };
    float orbitDistance = 10.0f;

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
