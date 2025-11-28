#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class PhysicalDevice;
class LogicalDevice;
class CommandBuffer;
class CommandPool;
class Queue;

class Buffer {
public:
    // TODO: maybe combine init/alloc and destroye/free ?
    void init(
        LogicalDevice device,
        VkBufferUsageFlags usage,
        size_t size,
        bool persistentMapping = false
    );
    void initFilled(
        LogicalDevice device, PhysicalDevice physicalDevice,
        VkBufferUsageFlags usage,
        CommandPool transferCommandPool, Queue transferQueue,
        size_t size, void *data
    );

    void alloc(LogicalDevice device, PhysicalDevice physicalDevice, VkMemoryPropertyFlags properties);
    void free(LogicalDevice device);
    void destroy(LogicalDevice device);

    VkBuffer get() { return buffer; };
    size_t getSize() { return size; };
    
    void fill(LogicalDevice device, void* data);
    static void copy(Buffer srcBuffer, Buffer dstBuffer, LogicalDevice device, CommandPool transferCommandPool, Queue transferQueue);

    // TODO: make a static version to bind multiple buffer
    void bindVertex(CommandBuffer commandBuffer);
    void bindIndex(CommandBuffer commandBuffer, VkIndexType indexType);

    static uint32_t findMemoryType(PhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

    bool persistentMapping;
private:
    VkBuffer buffer;
    VkDeviceMemory bufferMemory;
    VkBufferUsageFlags usage;
    size_t size;

    void* bufferMapped;
};
