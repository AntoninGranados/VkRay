#pragma once

#include "./../app_context.hpp"
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
    void buildPipeline(AppContext& ctx);

    void render(AppContext& ctx);

private:
    Image images[2];
    ImageView imageViews[2];
    Sampler samplers[2];
    
    DescriptorSetLayout setLayout, screenSetLayout;
    descriptorSetList_t descriptorSets[2], screenDescriptorSets[2];
    GraphicsPipeline pipeline, screenPipeline;
    
    Buffer vertexBuffer, indexBuffer;
    bufferList_t pathtracingUniformBuffers, screenUniformBuffers;
    bufferList_t pixelInfoBuffers;

    ExportService exportService;

    size_t frame = 0;

    void renderMain(AppContext& ctx);
    void renderUiLayer(AppContext& ctx);
};
