#pragma once

#include <filesystem>

#include "app/app_context.hpp"
#include "./export_service.hpp"

#include "VkSmol/graph/builder_resource.hpp"

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
    bool promptOutputPath();

    void renderHeadless(AppContext& ctx, bool captureOutput = false);
    void saveCapture(AppContext& ctx, const std::filesystem::path& path);
    void resize(AppContext& ctx, uint32_t width, uint32_t height);

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

    // Windowed-only
    ImageHandle swapchainImageHandle;
    SubmissionGroupHandle uiGroupHandle;
    PassHandle displayPassHandle, uiPassHandle, presentPassHandle;
    GraphicsPipelineHandle displayPipelineHandle;
    BufferHandle vertexBufferHandle, indexBufferHandle;
    ExportService exportService;
    uint64_t lastSwapchainGeneration = 0;

    // Common
    ImageHandle previousPathtracingImageHandle, currentPathtracingImageHandle;
    ImageHandle outputImageHandle;

    SubmissionGroupHandle mainGroupHandle;

    PassHandle pathtracePassHandle, compositePassHandle;
    PassHandle exportPassHandle;

    ComputePipelineHandle pathtracingPipelineHandle, compositingPipelineHandle;

    BufferHandle pathtracingUBOHandle;
    BufferHandle displayUBOHandle;
    BufferHandle pixelInfoBufferHandle;

    void handleResize(AppContext& ctx, const VkExtent2D& extent);
    void pathtracingPass(AppContext& ctx, const FrameContext& frameContext, bool captureOutput = false);
    void uiPass(AppContext& ctx);
};
