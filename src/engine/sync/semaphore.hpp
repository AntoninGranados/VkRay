#pragma once

#include <vulkan/vulkan.h>

class LogicalDevice;

class Semaphore {
public:
    void init(LogicalDevice device);
    void destroy(LogicalDevice device);
    VkSemaphore get() { return semaphore; };

private:
    VkSemaphore semaphore;
};
