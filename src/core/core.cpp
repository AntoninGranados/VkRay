#include "core.hpp"

#include <memory>
#include <string_view>

#include <glm/glm.hpp>

#include "core/camera/aperture.hpp"
#include "core/fields/parameter_serializer.hpp"
#include "core/render/resource_shader_file_system.hpp"
#include "utils/resources.hpp"

Core::Core(const std::optional<std::string>& windowTitle, uint32_t version)
    : window(windowTitle ? std::optional<VkSmol::Window>(std::in_place, *windowTitle, kWindowWidth, kWindowHeight)
                         : std::nullopt),
      engine("VkRay", version, window ? std::optional<VkSmol::Window&>(*window) : std::nullopt,
             VkSmol::EngineFeatures{.imageArrays = true}),
      graph(engine) {}

void Core::init(const std::optional<std::string>& windowTitle, uint32_t version) {
    static Core core(windowTitle, version);
    instance = &core;
    core.engine.setShaderFileSystem(std::make_shared<ResourceShaderFileSystem>());
    loadParameters();

    core.lensImageHandle = core.engine.createImage("LensTexture", VK_FORMAT_R8_UNORM, aperture::kSize, aperture::kSize);

    const std::string_view blueNoise = Resources::find("builtin:/noise/blue_noise.bin").value();
    core.blueNoiseBufferHandle = core.engine.createBuffer("BlueNoiseBuffer", blueNoise.size(), blueNoise.data());

    const glm::ivec2 renderSize = core.parameters.get<glm::ivec2>("renderer/output/render_size");
    const VkExtent2D extent = {static_cast<uint32_t>(renderSize.x), static_cast<uint32_t>(renderSize.y)};
    core.coreRenderer.init(extent, "", core.lensImageHandle, core.blueNoiseBufferHandle);
    core.stages.push_back(&core.coreRenderer);
}

void Core::loadParameters() { get().parameters = ParameterSerializer::load("builtin:/config/parameters.json"); }

bool Core::consumeRenderDirty() {
    Core& c = get();
    if (!c.renderDirty) return false;
    c.renderDirty = false;
    c.coreRenderer.restartAccumulation();
    return true;
}

bool Core::consumeResize() {
    Core& c = get();
    if (c.targetExtent.width == 0) return false;
    VkExtent2D current = c.coreRenderer.getRenderExtent();
    if (c.targetExtent.width == current.width && c.targetExtent.height == current.height) return false;
    c.coreRenderer.resize(c.targetExtent.width, c.targetExtent.height);
    return true;
}

void Core::updateAnimationDirty(Core& c) {
    if (c.animation.sample()) markRenderDirty();
    if (!c.animation.isPaused()) markRenderDirty();
    consumeRenderDirty();
}

void Core::buildGraph() {
    Core& c = get();
    c.graphRebuildRequested = false;
    c.graph.reset();
    for (RenderStage* stage : c.stages) stage->addPasses(c.graph);
    c.graph.bake();
}

int Core::getTargetSampleCount(Core& c) {
    if (c.renderMode != RenderMode::Preview) return c.parameters.get<int>("renderer/sampling/render_samples");
    if (c.parameters.get<ViewportShading>("renderer/viewport/shading") == ViewportShading::Rasterized) return 0;
    const int maxSamples = c.parameters.get<int>("renderer/viewport/max_samples");
    return maxSamples > 0 ? maxSamples : SampleAccumulator::kUnboundedSamples;
}

void Core::renderFrame() {
    Core& c = get();
    Scene& scene = c.coreRenderer.getScene();
    c.fileWatcher.poll();

    updateAnimationDirty(c);
    scene.runPreRender();
    for (RenderStage* stage : c.stages) stage->update();

    if (c.graphRebuildRequested) buildGraph();

    c.coreRenderer.setTargetSampleCount(getTargetSampleCount(c));
    for (RenderStage* stage : c.stages) stage->prepare();
    c.graph.execute();
}

void Core::startRender() {
    Core& c = get();
    if (c.renderMode != RenderMode::Preview) return;
    c.renderMode = RenderMode::RenderSingle;
    auto renderSize = c.parameters.get<glm::ivec2>("renderer/output/render_size");
    requestResize(renderSize.x, renderSize.y);
    markRenderDirty();
}

void Core::startRenderAnim() {
    Core& c = get();
    if (c.renderMode != RenderMode::Preview) return;
    c.renderMode = RenderMode::RenderAnimation;
    auto renderSize = c.parameters.get<glm::ivec2>("renderer/output/render_size");
    requestResize(renderSize.x, renderSize.y);
    markRenderDirty();
    c.animation.reset(0);
}
