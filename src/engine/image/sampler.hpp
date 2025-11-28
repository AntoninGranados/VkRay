#pragma once

#include <vulkan/vulkan.h>

class LogicalDevice;
class PhysicalDevice;


class Sampler {
public:
    void init(LogicalDevice device, PhysicalDevice physicalDevice);
    void destroy(LogicalDevice device);

    VkSampler get() { return sampler; }
private:
    VkSampler sampler;
};
