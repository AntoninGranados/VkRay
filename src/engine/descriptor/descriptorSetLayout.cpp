#include "descriptorSetLayout.hpp"

#include "../logicalDevice.hpp"
#include "../utils.hpp"

void DescriptorSetLayout::init(LogicalDevice device) {
    VkDescriptorSetLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    createInfo.pBindings = bindings.data();

    vkCheck(vkCreateDescriptorSetLayout(device.get(), &createInfo, nullptr, &descritorSetLayout), "Failed to create descriptor set layout");
}

void DescriptorSetLayout::destroy(LogicalDevice device) {
    vkDestroyDescriptorSetLayout(device.get(), descritorSetLayout, nullptr);
}

void DescriptorSetLayout::addBinding(VkShaderStageFlags shaderStage, VkDescriptorType descriptorType, uint32_t descriptorCount) {
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = static_cast<uint32_t>(bindings.size());
    layoutBinding.stageFlags = shaderStage;
    layoutBinding.descriptorType = descriptorType;
    layoutBinding.descriptorCount = descriptorCount;

    bindings.push_back(layoutBinding);
}