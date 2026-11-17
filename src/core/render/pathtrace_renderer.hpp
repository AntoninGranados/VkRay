#pragma once

#include <array>
#include <string>
#include <vector>

#include "VkSmol/graph/builder_resource.hpp"
#include "VkSmol/graph/render_graph.hpp"

#include "core/ecs/system_scheduler.hpp"
#include "core/ecs/systems/gpu_packing_system.hpp"
#include "core/render/render_stage.hpp"
#include "core/render/sample_accumulator.hpp"
#include "core/render_structures.hpp"
#include "core/scene/scene.hpp"

struct RenderResources {
    VkSmol::ImageHandle outputImageHandle = {};
    VkSmol::BufferHandle pixelInfoBufferHandle = {};
    SceneGpuBuffers sceneHandles = {};
};

class PathtraceRenderer : public RenderStage {
public:
    void init(VkExtent2D extent, const std::string& passTag, VkSmol::ImageHandle lensImageHandle,
              VkSmol::BufferHandle blueNoiseBufferHandle);
    void update() override;
    void addPasses(VkSmol::RenderGraph& graph) override;

    uint32_t getSampleCount() { return accumulator.getSampleCount(); }
    void setTargetSampleCount(int n) { accumulator.setTargetSampleCount(n); }
    void restartAccumulation() { accumulator.restart(); }
    bool isRenderFinished() { return accumulator.isRenderFinished(); }
    void prepare() override;
    void resize(uint32_t width, uint32_t height);

    VkExtent2D getRenderExtent() const { return renderExtent; }
    std::string getPathtracingPassName() const { return tag + "PathtracingPass"; }
    std::string getCompositingPassName() const { return tag + "CompositingPass0"; }
    VkSmol::ImageHandle getOutputImageHandle() const { return resources.outputImageHandle; }
    const RenderResources& getResources() const { return resources; }
    void shareTextures(const SceneGpuBuffers& source) {
        resources.sceneHandles.textures = source.textures;
        resources.sceneHandles.sharesTextures = true;
    }

    float getFrustumScale() const { return frustumScale; }
    void setFrustumScale(float scale) { frustumScale = scale; }

    Scene& getScene() { return scene; }

private:
    Scene scene;
    std::string tag;

    RenderResources resources = {};

    VkSmol::ImageHandle lensImageHandle;
    VkSmol::BufferHandle blueNoiseBufferHandle;
    VkSmol::ImageHandle previousPathtracingImageHandle, currentPathtracingImageHandle;
    std::array<VkSmol::ImageHandle, 2> compositingPingHandles;

    PathtracerUBO pathtracerUBO;
    VkSmol::BufferHandle pathtracingUBOHandle;
    std::vector<VkSmol::BufferHandle> compositingPassUBOHandles;

    VkExtent2D renderExtent = {};
    float frustumScale = 1.0f;
    bool dispatchPathtracing = false;

    ecs::SystemScheduler packingScheduler;
    SampleAccumulator accumulator;
    uint64_t seenChangeTick = 0;
};
