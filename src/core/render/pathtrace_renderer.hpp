#pragma once

#include <string>

#include "VkSmol/engine.hpp"
#include "VkSmol/graph/builder_resource.hpp"

#include "core/ecs/entity.hpp"
#include "core/render/sample_accumulator.hpp"
#include "core/scene/scene.hpp"
#include "core/render_structures.hpp"

struct FrameContext;

struct RenderResources {
    ImageHandle outputImageHandle = {};
    BufferHandle pixelInfoBufferHandle = {};
    SceneGpuBuffers sceneHandles = {};
};

class PathtraceRenderer {
public:
    virtual ~PathtraceRenderer() = default;

    RenderResources initGraph(RenderGraphBuilder& builder, VkExtent2D extent, const std::string& tag, ImageHandle lensImageHandle);

    uint32_t getSampleCount()            { return accumulator.getSampleCount(); }
    void     setTargetSampleCount(int n) { accumulator.setTargetSampleCount(n); }
    void     restartAccumulation()       { accumulator.restart(); }
    bool isRenderFinished() { return accumulator.isRenderFinished(); }
    void setLightMode(LightMode mode) { pathtracerUBO.render.lightMode = mode; }
    void render(const FrameContext& frameContext);
    void resize(uint32_t width, uint32_t height);

    VkExtent2D getRenderExtent() const { return renderExtent; }
    TimestampHandle getPathtracingTimestamp() const { return pathtracingTimestamp; }
    TimestampHandle getCompositingTimestamp() const { return compositingTimestamp; }
    ImageHandle getOutputImageHandle() const { return resources.outputImageHandle; }

    Scene& getScene() { return scene; }

protected:
    PathtracerUBO  pathtracerUBO;
    CompositingUBO compositingUBO;

    SubmissionGroupHandle getGroupHandle() const { return groupHandle; }

    virtual void onAfterDispatch([[maybe_unused]] CommandBuffer& commandBuffer) {}
    virtual void onResize([[maybe_unused]] uint32_t width, [[maybe_unused]] uint32_t height) {}

private:
    void setDefaultUBOs();

    Scene scene;

    RenderResources resources = {};

    ImageHandle previousPathtracingImageHandle, currentPathtracingImageHandle;

    BufferHandle pathtracingUBOHandle;
    BufferHandle compositingUBOHandle;

    PassHandle pathtracePassHandle;
    PassHandle compositePassHandle;

    TimestampHandle pathtracingTimestamp;
    TimestampHandle compositingTimestamp;

    SubmissionGroupHandle groupHandle = {};

    VkExtent2D renderExtent = {};

    SampleAccumulator accumulator;
};
