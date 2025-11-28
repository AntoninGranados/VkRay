#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

class LogicalDevice;
class CommandPool;
class Queue;

class CommandBuffer {
public:
    void alloc(LogicalDevice device, CommandPool pool);
    void free(LogicalDevice device, CommandPool pool);
    VkCommandBuffer get() { return commandBuffer; };

    void beginRecording(VkCommandBufferUsageFlagBits flags = (VkCommandBufferUsageFlagBits)0);
    void endRecording();
    void reset();

    static CommandBuffer beginSingleTimeCommands(LogicalDevice device, CommandPool pool);
    static void endSingleTimeCommands(LogicalDevice device, CommandPool pool, Queue queue, CommandBuffer &commandBuffer);

private:
    VkCommandBuffer commandBuffer;
};