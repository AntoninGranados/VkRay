#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

class LogicalDevice;

class DescriptorPool {
public:
    void init(LogicalDevice device);
    void destroy(LogicalDevice device);
    VkDescriptorPool get() { return descriptorPool; };

    void addPoolSize(VkDescriptorType descriptorType, uint32_t descriptorCount);

private:
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorPoolSize> poolSizes;
    uint32_t maxSize = 0; // we will only be hable to allocate the number specified in pool sizes
};
