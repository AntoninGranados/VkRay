#pragma once

#include "app/app_context.hpp"
#include "./export_service.hpp"

#include "./imgui/imgui.h"
#include "./imgui/ImGuizmo.h"


typedef uint16_t index_t;

struct ScreenVertex {
    alignas(16) glm::vec2 pos;
};

const std::vector<ScreenVertex> vertices = {
    { .pos = {  1.0f, 1.0f } },
    { .pos = {  1.0f,-1.0f } },
    { .pos = { -1.0f,-1.0f } },
    { .pos = { -1.0f, 1.0f } }
};

const std::vector<index_t> indices = {
    0, 1, 2, 2, 3, 0
};

class RenderHandler {
public:
    void init(AppContext& ctx);
    void destroy(AppContext& ctx);
    void buildPipelines(AppContext& ctx);

    void render(AppContext& ctx);

private:
    Image pathtracingImages[2];
    ImageView pathtracingImageViews[2];
    Sampler pathtracingSamplers[2];
    Image outputImage;
    ImageView outputImageView;
    Sampler outputSampler;
    
    DescriptorSetLayout pathtracingSetLayout, compositingSetLayout, displaySetLayout;
    descriptorSetList_t pathtracingDescriptorSets[2], compositingDescriptorSets[2], displayDescriptorSets[2];
    GraphicsPipeline pathtracingPipeline, compositingPipeline, displayPipeline;
    
    Buffer vertexBuffer, indexBuffer;
    bufferList_t pathtracingUniformBuffers, displayUniformBuffers;
    bufferList_t pixelInfoBuffers;

    ExportService exportService;

    size_t frame = 0;

    void pathtracingPass(AppContext& ctx);
    void uiPass(AppContext& ctx);
};
