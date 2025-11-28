#include "commandBuffer.hpp"

#include "./commandPool.hpp"
#include "../logicalDevice.hpp"
#include "../queue.hpp"
#include "../utils.hpp"

void CommandBuffer::alloc(LogicalDevice device, CommandPool pool) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = pool.get();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;  // Needs to be a component (variable)
    allocInfo.commandBufferCount = 1;   // Would need a static function to allocation multiple command buffers and return a std::vector

    vkCheck(vkAllocateCommandBuffers(device.get(), &allocInfo, &commandBuffer), "Failed to allocate command buffer");
}

void CommandBuffer::free(LogicalDevice device, CommandPool pool) {
    vkFreeCommandBuffers(device.get(), pool.get(), 1, &commandBuffer);
}

void CommandBuffer::beginRecording(VkCommandBufferUsageFlagBits flags) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = flags;

    vkCheck(vkBeginCommandBuffer(commandBuffer, &beginInfo), "Failed to begin recording command buffer");
}

void CommandBuffer::endRecording() {
    vkCheck(vkEndCommandBuffer(commandBuffer), "Failed to record command buffer");
}

void CommandBuffer::reset() {
    vkResetCommandBuffer(commandBuffer, 0);
}

CommandBuffer CommandBuffer::beginSingleTimeCommands(LogicalDevice device, CommandPool pool) {
    CommandBuffer commandBuffer;
    commandBuffer.alloc(device, pool);
    commandBuffer.beginRecording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    return commandBuffer;
}

void CommandBuffer::endSingleTimeCommands(LogicalDevice device, CommandPool pool, Queue queue, CommandBuffer &commandBuffer) {
    commandBuffer.endRecording();

    queue.submit({ commandBuffer });
    queue.wait();

    commandBuffer.free(device, pool);
}
