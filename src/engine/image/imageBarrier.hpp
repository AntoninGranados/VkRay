#pragma once

#include <vulkan/vulkan.h>

class CommandBuffer;

class ImageBarrier {
public:
    void call(
        CommandBuffer commandBuffer,
        VkImage image,
        VkImageLayout srcLayout, VkImageLayout dstLayout,
        VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
        VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask
    );

    VkImageMemoryBarrier get() { return barrier; }

private:
    VkImageMemoryBarrier barrier;
};
