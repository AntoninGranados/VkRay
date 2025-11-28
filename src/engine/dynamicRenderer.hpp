#pragma once

#include <vulkan/vulkan.h>

class CommandBuffer;
class Swapchain;
class LogicalDevice;

class DynamicRenderer {
public:
    void init(LogicalDevice device);
    
    void begin(
        CommandBuffer commandBuffer,
        VkImageView imageView, VkImageLayout imageLayout,
        VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp,
        VkClearColorValue clearColor, VkRect2D renderArea
    );
    void end(CommandBuffer commandBuffer);

private:
    PFN_vkCmdBeginRenderingKHR vkCmdBeginRenderingKHR;
    PFN_vkCmdEndRenderingKHR vkCmdEndRenderingKHR;
};
