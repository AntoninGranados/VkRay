#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

class LogicalDevice;

class DescriptorSetLayout {
public:
    void init(LogicalDevice device);
    void destroy(LogicalDevice device);
    VkDescriptorSetLayout get() { return descritorSetLayout; };
    std::vector<VkDescriptorSetLayoutBinding> getBindings() { return bindings; };

    void addBinding(VkShaderStageFlags shaderStage, VkDescriptorType descriptorType, uint32_t descriptorCount = 1);

private:
    VkDescriptorSetLayout descritorSetLayout;
    std::vector<VkDescriptorSetLayoutBinding> bindings;
};
