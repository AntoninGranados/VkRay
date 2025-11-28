#include "semaphore.hpp"

#include "../logicalDevice.hpp"
#include "../utils.hpp"

void Semaphore::init(LogicalDevice device) {
    VkSemaphoreCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vkCheck(vkCreateSemaphore(device.get(), &createInfo, nullptr, &semaphore), "Failed to create semaphore");
}

void Semaphore::destroy(LogicalDevice device) {
    vkDestroySemaphore(device.get(), semaphore, nullptr);
}
