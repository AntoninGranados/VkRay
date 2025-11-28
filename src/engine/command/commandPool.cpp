#include "commandPool.hpp"

#include "../logicalDevice.hpp"
#include "../queue.hpp"
#include "../utils.hpp"

void CommandPool::init(LogicalDevice device, uint32_t queueFamilyIndex) {
    VkCommandPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    createInfo.queueFamilyIndex = queueFamilyIndex;

    vkCheck(vkCreateCommandPool(device.get(), &createInfo, nullptr, &commandPool), "Failed to create command pool");
}

void CommandPool::destroy(LogicalDevice device) {
    vkDestroyCommandPool(device.get(), commandPool, nullptr);
}
