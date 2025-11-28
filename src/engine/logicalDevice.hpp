#pragma once

#include <vulkan/vulkan.h>

#include <set>
#include <vector>

class PhysicalDevice;

class LogicalDevice {
public:
    void init(
        PhysicalDevice physicalDevice,
        std::vector<uint32_t> queueFamilyIndices,
        std::vector<const char*> extensions,
        std::vector<const char*> validationLayers
    );
    void destroy();
    VkDevice get() { return device; };

    void waitIdle();

private:
    VkDevice device;
};
