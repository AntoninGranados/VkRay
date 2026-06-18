#pragma once

#include "app/app_context.hpp"
#include "./export_service.hpp"

#include "engine/graph/builder_resource.hpp"

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

    ImageHandle swapchainImageHandle;
    ImageHandle previousPathtracingImageHandle, currentPathtracingImageHandle;
    ImageHandle outputImageHandle;

    SubmissionGroupHandle mainGroupHandle, uiGroupHandle;

    PassHandle pathtracePassHandle, compositePassHandle, displayPassHandle;
    PassHandle uiPassHandle;
    PassHandle exportPassHandle;
    PassHandle presentPassHandle;

    // =============================================================

    ComputePipelineHandle  pathtracingPipelineHandle, compositingPipelineHandle;
    GraphicsPipelineHandle displayPipelineHandle;

    BufferHandle vertexBufferHandle, indexBufferHandle;
    BufferHandle pathtracingUBOHandle;
    BufferHandle displayUBOHandle;
    BufferHandle pixelInfoBufferHandle;

    ExportService exportService;

    uint64_t lastSwapchainGeneration = 0;

    void handleResize(AppContext& ctx, const VkExtent2D& extent);

    void pathtracingPass(AppContext& ctx, const FrameContext& frameContext);
    void uiPass(AppContext& ctx);
};
