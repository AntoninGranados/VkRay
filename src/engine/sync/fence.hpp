#pragma once

#include <vulkan/vulkan.h>

#include <vector>

class LogicalDevice;

class Fence {
public:
    // The fence is not signaled by default
    void init(LogicalDevice device, bool signaled = false);
    void destroy(LogicalDevice device);
    VkFence get() { return fence; };
    
    static void wait(std::vector<Fence> fences, LogicalDevice device, uint64_t timeout = UINT64_MAX);
    void wait(LogicalDevice device, uint64_t timeout = UINT64_MAX);
    static void reset(std::vector<Fence> fences, LogicalDevice device);
    void reset(LogicalDevice device);

private:
    VkFence fence;
};
