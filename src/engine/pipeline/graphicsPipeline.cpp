#include "graphicsPipeline.hpp"

#include "../command/commandBuffer.hpp"
#include "../descriptor/descriptorSetLayout.hpp"
#include "../logicalDevice.hpp"
#include "../shader.hpp"
#include "../utils.hpp"

void GraphicsPipeline::init(
    LogicalDevice device,
    VkPipelineRenderingCreateInfoKHR pipelineCreate,
    VkPipelineVertexInputStateCreateInfo vertexInputInfo,
    std::vector<Shader> shaders,
    std::vector<DescriptorSetLayout> setLayouts,
    GraphicsPipeline other
    // VkViewport viewport, VkRect2D scissor
) {
    // Input Assembly State
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Dynamic State
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // Viewport State
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    // viewportState.pViewports = &viewport;    // If we don't use dynamic state
    viewportState.scissorCount = 1;
    // viewportState.pScissors = &scissor;      // If we don't use dynamic state

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;         // `VK_TRUE` will clamp fragment that are outside of the near/far planes — useful for shadow maping
    rasterizer.rasterizerDiscardEnable = VK_FALSE;  // `VK_TRUE` disable the rasterizer
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;  // Needs a special feature enabled if we don't use fill
    rasterizer.lineWidth = 1.0f;                    // Needs the wideLine feature if > 1.0f
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;          // `VK_TRUE` for shadow maping

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Color Blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // Pipeline layout
    std::vector<VkDescriptorSetLayout> vkSetLayouts;
    for (DescriptorSetLayout setLayout : setLayouts)
        vkSetLayouts.emplace_back(setLayout.get());

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(vkSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = vkSetLayouts.data();
    vkCheck(vkCreatePipelineLayout(device.get(), &pipelineLayoutInfo, nullptr, &pipelineLayout), "Failed to create pipeline layout");

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    for (Shader shader : shaders)
        shaderStages.emplace_back(shader.getShaderStageCreateInfo());

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.subpass = 0;
    pipelineInfo.renderPass = VK_NULL_HANDLE;   //? for dynamic rendering
    pipelineInfo.pNext = &pipelineCreate;       //? for dynamic rendering
    
    // To rebuild the pipeline more efficiently (using another pipeline)
    pipelineInfo.flags |= VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT; //! should not always be active
    if (other.get() != VK_NULL_HANDLE) {
        pipelineInfo.basePipelineIndex = -1;
        pipelineInfo.basePipelineHandle = other.get();
        pipelineInfo.flags |= VK_PIPELINE_CREATE_DERIVATIVE_BIT;
    }

    vkCheck(vkCreateGraphicsPipelines(device.get(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline), "Failed to create graphic pipeline");

}

void GraphicsPipeline::destroy(LogicalDevice device) {
    vkDestroyPipeline(device.get(), pipeline, nullptr);
    vkDestroyPipelineLayout(device.get(), pipelineLayout, nullptr);
}

void GraphicsPipeline::bind(CommandBuffer commandBuffer) {
    vkCmdBindPipeline(commandBuffer.get(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

void GraphicsPipeline::setViewport(CommandBuffer commandBuffer, VkViewport viewport) {
    vkCmdSetViewport(commandBuffer.get(), 0, 1, &viewport);
}

void GraphicsPipeline::setScissor(CommandBuffer commandBuffer, VkRect2D scissor) {
    vkCmdSetScissor(commandBuffer.get(), 0, 1, &scissor);
}

void GraphicsPipeline::draw(CommandBuffer commandBuffer, size_t vertexCount) {
    vkCmdDraw(commandBuffer.get(), vertexCount, 1, 0, 0);
}

void GraphicsPipeline::drawIndexed(CommandBuffer commandBuffer, size_t indexCount) {
    vkCmdDrawIndexed(commandBuffer.get(), indexCount, 1, 0, 0, 0);
}
