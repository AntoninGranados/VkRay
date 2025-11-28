#pragma once

#include <vulkan/vulkan.h>

#include <vector>

class LogicalDevice;
class Buffer;
class CommandBuffer;
class ImageView;
class Sampler;
class DescriptorSetLayout;
class DescriptorPool;

class DescriptorSet {
public:
    void alloc(LogicalDevice device, DescriptorPool descriptorPool, DescriptorSetLayout descriptorSetLayout);
    // TODO: FIX IT
    static void alloc(
        std::vector<DescriptorSet>& descriptorSets,
        LogicalDevice device,
        DescriptorPool descriptorPool,
        std::vector<DescriptorSetLayout> descriptorSetLayouts
    );
    // void free(LogicalDevice device);
    VkDescriptorSet get() { return descriptorSet; };

    void linkUniformBuffer(Buffer buffer, uint32_t binding);
    void linkStorageBuffer(Buffer buffer, uint32_t binding);
    void linkCombinedImageSampler(ImageView view, Sampler sampler, uint32_t binding);
    void update(LogicalDevice device);
    void bind(CommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS);

private:
    VkDescriptorSet descriptorSet;
    
    std::vector<std::pair<VkDescriptorBufferInfo, uint32_t> > bufferInfos;
    std::vector<std::pair<VkDescriptorBufferInfo, uint32_t> > storageInfos;
    std::vector<std::pair<VkDescriptorImageInfo, uint32_t> > imageInfos;
    std::vector<VkWriteDescriptorSet> descriptorWrites;
};
