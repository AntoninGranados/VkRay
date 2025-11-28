#pragma once

#include <vulkan/vulkan.h>

#include <functional>

class Instance;

class PhysicalDevice {
public:
    PhysicalDevice(VkPhysicalDevice physicalDevice);
    PhysicalDevice();

    void init(Instance instance);
    void init(Instance instance, std::function<int(PhysicalDevice)> scoringFunction);
    // void destroy();  //? We don't need to destroy the physical device
    VkPhysicalDevice get() { return physicalDevice; };

private:
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

    static int scoreDeviceDefault(PhysicalDevice device);
};