#include "fence.hpp"

#include "../logicalDevice.hpp"
#include "../utils.hpp"

void Fence::init(LogicalDevice device, bool signaled) {
    VkFenceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    // if (signaled)
    createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    vkCheck(vkCreateFence(device.get(), &createInfo, nullptr, &fence), "Failed to create fence");
}

void Fence::destroy(LogicalDevice device) {
    vkDestroyFence(device.get(), fence, nullptr);
}

void Fence::wait(std::vector<Fence> fences, LogicalDevice device, uint64_t timeout) {
    std::vector<VkFence> vkFences;
    for (Fence& fence : fences)
        vkFences.emplace_back(fence.get());
    vkWaitForFences(device.get(), static_cast<uint32_t>(vkFences.size()), vkFences.data(), VK_TRUE, timeout);
}

void Fence::wait(LogicalDevice device, uint64_t timeout) {
    vkWaitForFences(device.get(), 1, &fence, VK_TRUE, timeout);
}

void Fence::reset(std::vector<Fence> fences, LogicalDevice device) {
    std::vector<VkFence> vkFences;
    for (Fence& fence : fences)
        vkFences.emplace_back(fence.get());
    vkResetFences(device.get(), static_cast<uint32_t>(vkFences.size()), vkFences.data());
}

void Fence::reset(LogicalDevice device) {
    vkResetFences(device.get(), 1, &fence);
}
