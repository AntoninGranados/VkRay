#pragma once

#include <string>

#include "app/app_context.hpp"
#include "./export_service.hpp"

#include "VkSmol/graph/builder_resource.hpp"
#include "VkSmol/memory/buffer.hpp"

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

    // Windowed: called each frame from Application::run()
    void render(AppContext& ctx);

    // Headless: render one accumulated sample; captureOutput copies to readback on last frame
    void renderHeadless(AppContext& ctx, bool captureOutput = false);
    // Headless: write the captured frame to a PNG file
    void saveCapture(AppContext& ctx, const std::string& path, uint32_t width, uint32_t height);

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

    // Headless-only
    Buffer readbackBuffer;

    void handleResize(AppContext& ctx, const VkExtent2D& extent);
    void pathtracingPass(AppContext& ctx, const FrameContext& frameContext, bool captureOutput = false);
    void uiPass(AppContext& ctx);
};
