#pragma once

#include <filesystem>

#include "VkSmol/engine.hpp"
#include "VkSmol/graph/builder_resource.hpp"

#include "core/scene/scene.hpp"
#include "export_service.hpp"
#include "core/camera.hpp"
#include "core/parameter_handler.hpp"

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

    bool     isRenderFinished()          { return targetSampleCount >= 0 && sampleCount >= static_cast<uint32_t>(targetSampleCount); }
    uint32_t getSampleCount()            { return sampleCount; }
    void     setTargetSampleCount(int n) { targetSampleCount = n; }
    void     reset()                     { sampleCount = 0; }
    void setCamera(const Camera& camera);
    void render(VkSmol& engine, const FrameContext& frameContext);
    
    void saveCapture(VkSmol& engine, const std::filesystem::path& path);
    void resize(VkSmol& engine, uint32_t width, uint32_t height);

    void bindParameters(ParameterHandler& params);
    void setDebugView(int v) { pathtracerUBO.render.debugView = v; }
    
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

    uint32_t sampleCount = 0;
    int      targetSampleCount = -1;
};
