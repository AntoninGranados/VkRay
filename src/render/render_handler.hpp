#pragma once

#include "app/app_context.hpp"
#include "./export_service.hpp"

#include "engine/graph/render_graph_executor.hpp"

struct FrameContext;

typedef uint16_t index_t;

struct ScreenVertex {
    alignas(16) glm::vec2 pos;
};

class RenderHandler {
public:
    void init(AppContext& ctx);
    void destroy(AppContext& ctx);
    void buildPipelines(AppContext& ctx);

    void render(AppContext& ctx);

private:
    const std::vector<ScreenVertex> vertices = {
        { .pos = {  1.0f, 1.0f } },
        { .pos = {  1.0f,-1.0f } },
        { .pos = { -1.0f,-1.0f } },
        { .pos = { -1.0f, 1.0f } }
    };

    const std::vector<index_t> indices = {
        0, 1, 2, 2, 3, 0
    };

    RenderGraphExecutor executor;

    ImageHandle swapchainImageHandle;
    ImageHandle previousPathtracingImageHandle;
    ImageHandle currentPathtracingImageHandle;
    ImageHandle outputImageHandle;

    PassHandle pathtracePassHandle;
    PassHandle compositePassHandle;
    PassHandle exportPassHandle;
    PassHandle displayPassHandle;
    PassHandle uiPassHandle;
    PassHandle presentPassHandle;

    // =============================================================

    DescriptorSetLayout pathtracingSetLayout, compositingSetLayout, displaySetLayout;
    DescriptorSetGroup pathtracingDescriptorSet, compositingDescriptorSet, displayDescriptorSet;
    ComputePipeline pathtracingPipeline, compositingPipeline;
    GraphicsPipeline displayPipeline;

    Buffer vertexBuffer, indexBuffer;
    PerFrameBuffer<PathtracerUBO> pathtracingUniformBuffers;
    PerFrameBuffer<ScreenUBO> displayUniformBuffers;
    SharedBuffer<PixelInfo> pixelInfoBuffer;

    ExportService exportService;

    uint64_t lastSwapchainGeneration = 0;

    void destroyDescriptors(AppContext& ctx);
    void rebuildDescriptors(AppContext& ctx);

    void writePathtracingDescriptors(AppContext& ctx, uint32_t frameIndex);
    void writeCompositingDescriptors(AppContext& ctx, uint32_t frameIndex);
    void writeDisplayDescriptors(AppContext& ctx, uint32_t frameIndex);

    void handleResize(AppContext& ctx, const VkExtent2D& extent);

    void pathtracingPass(AppContext& ctx, const FrameContext& frameContext);
    void uiPass(AppContext& ctx);
};
