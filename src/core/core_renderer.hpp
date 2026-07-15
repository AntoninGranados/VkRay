#pragma once

#include <filesystem>

#include "VkSmol/engine.hpp"
#include "VkSmol/graph/builder_resource.hpp"

#include "core/scene/scene.hpp"
#include "export_service.hpp"

struct FrameContext;

struct CoreResources {
    ImageHandle outputImageHandle = {};
    BufferHandle pixelInfoBufferHandle = {};
    SceneGpuBuffers sceneHandles = {};
};

class CoreRenderer {
public:
    CoreResources initGraph(RenderGraphBuilder& builder);
    void destroy();
    // TODO: make it non blocking (compile/build in the background and replace when finished)
    void buildPipelines();

    bool     isRenderFinished()          { return targetSampleCount >= 0 && sampleCount >= static_cast<uint32_t>(targetSampleCount); }
    uint32_t getSampleCount()            { return sampleCount; }
    void     setTargetSampleCount(int n) { targetSampleCount = n; }
    void     restartAccumulation()       { sampleCount = 0; }
    void render(const FrameContext& frameContext);

    void saveCapture(const std::filesystem::path& path);
    void resize(uint32_t width, uint32_t height);
    VkExtent2D getRenderExtent() const { return renderExtent; }

    void bindParameters();
    void setSelectedObjectId(int i) { selectedObjectId = i; }

private:
    CoreResources resources = {};

    ExportService exportService;

    ImageHandle previousPathtracingImageHandle, currentPathtracingImageHandle;
    
    PathtracerUBO  pathtracerUBO;
    CompositingUBO compositingUBO;
    BufferHandle pathtracingUBOHandle;
    BufferHandle compositingUBOHandle;
    
    AOVFlags aovFlags = {};

    PassHandle pathtracePassHandle;
    PassHandle compositePassHandle;
    PassHandle exportPassHandle;

    ComputePipelineHandle pathtracingPipelineHandle;
    ComputePipelineHandle compositingPipelineHandle;

    SubmissionGroupHandle coreGroupHandle = {};

    VkExtent2D renderExtent = {};

    uint32_t sampleCount = 0;
    int      targetSampleCount = -1;
    int      selectedObjectId = -1;
};
