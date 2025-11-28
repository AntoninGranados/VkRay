#pragma once

#include <vulkan/vulkan.h>

#include <string>

class LogicalDevice;
class PhysicalDevice;

class CommandBuffer;
class CommandPool;
class Queue;

class Buffer;


class Image {
public:
    void init(
        LogicalDevice device,
        uint32_t width, uint32_t height,
        VkFormat format, VkImageUsageFlags usage
    );
    void initFromPath(
        LogicalDevice device, PhysicalDevice physicalDevice,
        CommandPool transferCommandPool, Queue transferQueue,
        std::string path
    );
    void alloc(LogicalDevice device, PhysicalDevice physicalDevice, VkMemoryPropertyFlags properties);
    void fillFromBuffer(
        LogicalDevice device, CommandBuffer commandBuffer,
        Buffer buffer
    );
    void free(LogicalDevice device);
    void destroy(LogicalDevice device);

    VkImage get() { return image; }
    VkFormat getFormat() { return format; }

private:
    VkImage image;
    VkDeviceMemory imageMemory;
    VkFormat format;
    uint32_t width, height;
};
