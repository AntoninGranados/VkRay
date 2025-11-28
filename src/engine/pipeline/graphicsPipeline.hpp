#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

class LogicalDevice;
class Shader;
class CommandBuffer;
class DescriptorSetLayout;

// TODO: make it work with component (there are default values, but we can add/change components)
// TODO: keep track of the shader modules (in case we want to rebuild the pipeline with the same ones)
class GraphicsPipeline {
public:
    GraphicsPipeline() {};

    void init(
        LogicalDevice device,
        VkPipelineRenderingCreateInfoKHR pipelineCreate,
        VkPipelineVertexInputStateCreateInfo vertexInputInfo,
        std::vector<Shader> shaders,
        std::vector<DescriptorSetLayout> setLayouts,
        GraphicsPipeline other = GraphicsPipeline()
        // VkViewport viewport, VkRect2D scissor,
    );
    void destroy(LogicalDevice device);
    VkPipeline get() { return pipeline; };
    VkPipelineLayout getLayout() { return pipelineLayout; };

    void bind(CommandBuffer commandBuffer);
    void setViewport(CommandBuffer commandBuffer, VkViewport viewport); // In case it is set to be dynamic
    void setScissor(CommandBuffer commandBuffer, VkRect2D scissor);     // In case it is set to be dynamic
    
    void draw(CommandBuffer commandBuffer, size_t vertexCount);
    void drawIndexed(CommandBuffer commandBuffer, size_t indexCount);

private:
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
};
