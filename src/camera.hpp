#pragma once

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    Camera(glm::vec3 position, glm::vec3 direction);

    bool cursorPosCallback(GLFWwindow *window, double x, double y);
    bool scrollCallback(GLFWwindow *window, double xoffset, double yoffset);
    bool processInput(GLFWwindow *window, float deltaTime);
    
    float getTanHFov() { return glm::tan(glm::radians(fov) * 0.5); }
    float getFov() { return fov; }
    glm::vec3 getPosition() { return position; }
    glm::vec3 getDirection() { return direction; }
    glm::vec3 getUp() { return up; }
    glm::mat4 getView();
    glm::mat4 getProjection(GLFWwindow* window);

    bool isLocked() { return locked; }
    void toggleLock() { locked = !locked; }

    void resetMouse() { firstMouse = true; }

private:
    float fov = 80.0f;
    float tanHFov;

    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 up = { 0.0, 1.0, 0.0 };

    float yaw   = 90.0f;
    float pitch = 0.0f;

    float lastX = 0.0f;
    float lastY = 0.0f;
    bool firstMouse = true;

    float speed = 20.0f;
    float sensitivity = 0.2f;

    bool locked = true;
};
