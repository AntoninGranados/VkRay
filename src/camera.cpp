#include "camera.hpp"

#include <iostream>

Camera::Camera(glm::vec3 position, glm::vec3 direction): position(position), direction(direction) {}

bool Camera::cursorPosCallback(GLFWwindow *window, double x, double y) {
    if (locked) return false;

    bool change = false;

    if (firstMouse) {
        lastX = x;
        lastY = y;
        firstMouse = false;
    }

    float xoffset = x - lastX;
    float yoffset = lastY - y;
    if (xoffset != 0 || yoffset != 0) change = true;
    lastX = x;
    lastY = y;
    
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw   += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)  pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 dir;
    dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    dir.y = sin(glm::radians(pitch));
    dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction = glm::normalize(dir);

    return change;
}

bool Camera::scrollCallback(GLFWwindow *window, double xoffset, double yoffset) {
    fov -= (float)yoffset;
    if (fov < 1.0f) fov = 1.0f;
    if (fov > 160.0f) fov = 160.0f;

    return yoffset != 0;
}

bool Camera::processInput(GLFWwindow *window, float deltaTime) {
    float velocity = speed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        velocity /= 8.0f;
    }

    glm::vec3 right = glm::normalize(glm::cross(direction, up));
    
    bool change = false;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        position += direction * velocity;
        change = true;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        position -= direction * velocity;
        change = true;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        position -= right * velocity;
        change = true;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        position += right * velocity;
        change = true;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        position += up * velocity;
        change = true;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        position -= up * velocity;
        change = true;
    }
    
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
        if (locked) resetMouse();
        locked = false;
    } else {
        locked = true;
    }

    return change;
}

glm::mat4 Camera::getView() {
    glm::mat4 view = glm::lookAt(position, position+direction, up);
    return view;
}

glm::mat4 Camera::getProjection(GLFWwindow* window) {
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    float fovY = glm::radians(fov);

    glm::mat4 proj = glm::perspective(fovY, aspect, 1e-4f, 1e4f);
    return proj;
}
