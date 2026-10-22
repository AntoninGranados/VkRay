#pragma once

#include <array>
#include <string>
#include <vector>

#include "VkSmol/graph/builder_resource.hpp"
#include "VkSmol/graph/render_graph.hpp"

#include "core/ecs/systems/gpu_packing_system.hpp"
#include "core/render/sample_accumulator.hpp"
#include "core/render_structures.hpp"
#include "core/scene/scene.hpp"

struct RenderResources {
    VkSmol::ImageHandle outputImageHandle = {};
    VkSmol::BufferHandle pixelInfoBufferHandle = {};
    SceneGpuBuffers sceneHandles = {};
};

class PathtraceRenderer {
public:
    virtual ~PathtraceRenderer() = default;

    void init(VkExtent2D extent, const std::string& passTag, VkSmol::ImageHandle lensImageHandle,
              VkSmol::BufferHandle blueNoiseBufferHandle);
    void addPasses(VkSmol::RenderGraph& graph);

    uint32_t getSampleCount() { return accumulator.getSampleCount(); }
    void setTargetSampleCount(int n) { accumulator.setTargetSampleCount(n); }
    void restartAccumulation() { accumulator.restart(); }
    bool isRenderFinished() { return accumulator.isRenderFinished(); }
    void prepare();
    void resize(uint32_t width, uint32_t height);

    VkExtent2D getRenderExtent() const { return renderExtent; }
    std::string getPathtracingPassName() const { return tag + "PathtracingPass"; }
    std::string getCompositingPassName() const { return tag + "CompositingPass0"; }
    VkSmol::ImageHandle getOutputImageHandle() const { return resources.outputImageHandle; }
    const RenderResources& getResources() const { return resources; }

    Scene& getScene() { return scene; }

protected:
    PathtracerUBO pathtracerUBO;

    virtual void onResize([[maybe_unused]] uint32_t width, [[maybe_unused]] uint32_t height) {}

private:
    void setDefaultUBOs();

    Scene scene;
    std::string tag;

    RenderResources resources = {};

    VkSmol::ImageHandle lensImageHandle;
    VkSmol::BufferHandle blueNoiseBufferHandle;
    VkSmol::ImageHandle previousPathtracingImageHandle, currentPathtracingImageHandle;
    std::array<VkSmol::ImageHandle, 2> compositingPingHandles;

    VkSmol::BufferHandle pathtracingUBOHandle;
    std::vector<VkSmol::BufferHandle> compositingPassUBOHandles;

    VkExtent2D renderExtent = {};
    bool dispatchPathtracing = false;

    SampleAccumulator accumulator;
    uint64_t seenChangeTick = 0;
};
