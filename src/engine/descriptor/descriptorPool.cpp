#include "descriptorPool.hpp"

#include "../logicalDevice.hpp"
#include "../utils.hpp"

// TODO: maybe use a DescriptorSetLayout to construct the pool sizes
void DescriptorPool::init(LogicalDevice device) {
    VkDescriptorPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    createInfo.pPoolSizes = poolSizes.data();
    createInfo.maxSets = maxSize;
    createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    vkCheck(vkCreateDescriptorPool(device.get(), &createInfo, nullptr, &descriptorPool), "Failed to create descriptor pool");
}

void DescriptorPool::destroy(LogicalDevice device) {
    poolSizes.clear();
    maxSize = 0;
    vkDestroyDescriptorPool(device.get(), descriptorPool, nullptr);
}


void DescriptorPool::addPoolSize(VkDescriptorType descriptorType, uint32_t descriptorCount) {
    VkDescriptorPoolSize poolSize{};
    poolSize.type = descriptorType;
    poolSize.descriptorCount = descriptorCount;

    poolSizes.emplace_back(poolSize);
    maxSize += descriptorCount;
}
