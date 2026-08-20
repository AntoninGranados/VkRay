#pragma once

#include <cstdint>
#include <filesystem>

#include "VkSmol/engine.hpp"
#include "VkSmol/graph/builder_resource.hpp"

#include "core/render/sample_accumulator.hpp"
#include "core/scene/scene.hpp"
#include "export_service.hpp"

struct FrameContext;

struct RenderResources {
    ImageHandle outputImageHandle = {};
    BufferHandle pixelInfoBufferHandle = {};
    SceneGpuBuffers sceneHandles = {};
};

class SceneRenderer {
public:
    RenderResources initGraph(RenderGraphBuilder& builder);
    void destroy();
    // TODO: make it non blocking (compile/build in the background and replace when finished)
    void buildPipelines();

    bool     isRenderFinished() { return accumulator.isRenderFinished(); }
    uint32_t getSampleCount()   { return accumulator.getSampleCount(); }
    TimestampHandle getPathtracingTimestamp() const { return pathtracingTimestamp; }
    TimestampHandle getCompositingTimestamp() const { return compositingTimestamp; }
    void setTargetSampleCount(int n) { accumulator.setTargetSampleCount(n); }
    void restartAccumulation()       { accumulator.restart(); }
    void render(const FrameContext& frameContext, const Camera& camera);

    void saveCapture(const std::filesystem::path& path);
    void resize(uint32_t width, uint32_t height);
    VkExtent2D getRenderExtent() const { return renderExtent; }

    void bindParameters();
    ImageHandle getLensImageHandle() const { return lensImageHandle; }

private:
    RenderResources resources = {};

    ExportService exportService;

    ImageHandle previousPathtracingImageHandle, currentPathtracingImageHandle;
    ImageHandle lensImageHandle;

    PathtracerUBO  pathtracerUBO;
    CompositingUBO compositingUBO;
    BufferHandle pathtracingUBOHandle;
    BufferHandle compositingUBOHandle;

    AOVFlags aovFlags = {};

    PassHandle pathtracePassHandle;
    PassHandle compositePassHandle;
    PassHandle exportPassHandle;

    TimestampHandle pathtracingTimestamp;
    TimestampHandle compositingTimestamp;

    SubmissionGroupHandle coreGroupHandle = {};

    VkExtent2D renderExtent = {};

    SampleAccumulator accumulator;
};
