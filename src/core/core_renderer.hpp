#pragma once

#include <filesystem>
#include <functional>

#include "VkSmol/engine.hpp"
#include "VkSmol/graph/builder_resource.hpp"

#include "app/app_context.hpp"
#include "scene/scene.hpp"
#include "export_service.hpp"

struct FrameContext;

struct CoreResources {
    ImageHandle outputImageHandle = {};
    BufferHandle pixelInfoBufferHandle = {};
    SceneGpuBuffers sceneHandles = {};
};

class CoreRenderer {
public:
    CoreResources initGraph(VkSmol& engine, RenderGraphBuilder& builder);
    void destroy(VkSmol& engine);
    // TODO: make it non blocking (compile/build in the background and replace when finished)
    void buildPipelines(VkSmol& engine);

    void render(AppContext& ctx, const FrameContext& frameContext);
    void renderHeadless(AppContext& ctx, bool captureOutput = false);
    
    void saveCapture(AppContext& ctx, const std::filesystem::path& path);
    void resize(AppContext& ctx, uint32_t width, uint32_t height);

    void setOnRenderComplete(std::function<void()> fn) { onRenderComplete = std::move(fn); }
    
private:
    std::function<void()> onRenderComplete;

    CoreResources resources = {};

    ExportService exportService;

    ImageHandle previousPathtracingImageHandle, currentPathtracingImageHandle;
    
    BufferHandle pathtracingUBOHandle;
    BufferHandle compositingUBOHandle;

    PassHandle pathtracePassHandle;
    PassHandle compositePassHandle;
    PassHandle exportPassHandle;

    ComputePipelineHandle pathtracingPipelineHandle;
    ComputePipelineHandle compositingPipelineHandle;

    SubmissionGroupHandle coreGroupHandle = {};


    void handleResize(AppContext& ctx, const VkExtent2D& extent);
    void pathtracingPass(AppContext& ctx, const FrameContext& frameContext, bool captureOutput = false);
};
