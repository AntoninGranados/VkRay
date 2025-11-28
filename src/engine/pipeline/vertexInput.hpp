#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

// TODO: Only support one binding now
template <typename T>
class VertexInput {
public:
    void addAttributeDescription(VkFormat format, uint32_t offset);

    VkPipelineVertexInputStateCreateInfo get(uint32_t binding = 0);

private:
    VkVertexInputBindingDescription bindingDescription;
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

    void initBindingDescription(uint32_t binding);
    void initAttributeDescriptions(uint32_t binding);
};


/*
============== IMPLEMENTATION ==============
*/

template <typename T>
void VertexInput<T>::initBindingDescription(uint32_t binding) {
    bindingDescription.binding = binding;
    bindingDescription.stride = sizeof(T);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
}

template <typename T>
void VertexInput<T>::addAttributeDescription(VkFormat format, uint32_t offset) {
    VkVertexInputAttributeDescription attributeDescription{};
    attributeDescription.location = static_cast<uint32_t>(attributeDescriptions.size());
    attributeDescription.format = format;
    attributeDescription.offset = offset;
    attributeDescriptions.push_back(attributeDescription);
}

template <typename T>
void VertexInput<T>::initAttributeDescriptions(uint32_t binding) {
    for (VkVertexInputAttributeDescription& attributeDescription : attributeDescriptions)
        attributeDescription.binding = binding;
}

template <typename T>
VkPipelineVertexInputStateCreateInfo VertexInput<T>::get(uint32_t binding) {
    initBindingDescription(binding);
    initAttributeDescriptions(binding);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    return vertexInputInfo;
}
