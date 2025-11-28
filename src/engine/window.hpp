#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

class Window {
public:
    void init(int width, int height, std::string name, GLFWframebuffersizefun resizeCallback);
    void destroy();
    GLFWwindow* get() { return window; };

    bool shouldClose();
    void pauseWhileMinimized();

private:
    GLFWwindow* window;
};