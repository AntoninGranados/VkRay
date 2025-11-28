#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class LogicalDevice;

class CommandPool {
public:
    void init(LogicalDevice device, uint32_t queueFamilyIndex);
    void destroy(LogicalDevice device);
    VkCommandPool get() { return commandPool; };

private:
    VkCommandPool commandPool;
};