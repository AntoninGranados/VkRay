#include "core.hpp"

#include <cassert>
#include <memory>

#include "core/fields/parameter_serializer.hpp"
#include "core/render/resource_shader_file_system.hpp"

Core::Core(const std::optional<std::string>& windowTitle, uint32_t version)
    : window(windowTitle ? std::optional<VkSmol::Window>(std::in_place, *windowTitle, kWindowWidth, kWindowHeight)
                         : std::nullopt),
      engine("VkRay", version, window ? std::optional<VkSmol::Window&>(*window) : std::nullopt), graph(engine) {}

void Core::init(const std::optional<std::string>& windowTitle, uint32_t version) {
    static Core core(windowTitle, version);
    instance = &core;
    core.engine.setShaderFileSystem(std::make_shared<ResourceShaderFileSystem>());
    loadParameters();
    core.coreRenderer.bindParameters();
    core.coreRenderer.init();
}

void Core::loadParameters() { get().parameters = ParameterSerializer::load("builtin:/config/parameters.json"); }

void Core::restartAccumulation() {
    Core& c = get();
    c.coreRenderer.restartAccumulation();
    if (c.renderMode == RenderMode::Preview) {
        const int n = c.parameters.get<int>("renderer/viewport/max_samples");
        c.coreRenderer.setTargetSampleCount(n > 0 ? n : SampleAccumulator::kUnboundedSamples);
    }
}

bool Core::consumeRenderDirty() {
    Core& c = get();
    if (!c.renderDirty) return false;
    c.renderDirty = false;
    c.restartAccumulation();
    return true;
}

void Core::resize(int width, int height) {
    get().coreRenderer.resize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
}

bool Core::consumeResize() {
    Core& c = get();
    if (c.targetExtent.width == 0) return false;
    VkExtent2D current = c.coreRenderer.getRenderExtent();
    if (c.targetExtent.width == current.width && c.targetExtent.height == current.height) return false;
    resize(c.targetExtent.width, c.targetExtent.height);
    return true;
}

void Core::updateAnimationDirty(Core& c) {
    if (c.animation.sample()) markRenderDirty();
    if (!c.animation.isPaused()) markRenderDirty();
    consumeRenderDirty();
}

void Core::rebuildGraphIfRequested(Core& c) {
    if (!c.graphRebuildRequested) return;
    c.graphRebuildRequested = false;
    assert(c.graphBuilder);

    c.graphBuilder();
}

void Core::renderFrame(std::function<void()> onPrepare) {
    Core& c = get();
    Scene& scene = c.coreRenderer.getScene();
    c.fileWatcher.poll();

    rebuildGraphIfRequested(c);

    updateAnimationDirty(c);
    scene.runPreRender();
    scene.runOnRender();

    c.coreRenderer.prepare();
    if (onPrepare) onPrepare();
    c.graph.execute();
}

void Core::startRender() {
    Core& c = get();
    if (c.renderMode != RenderMode::Preview) return;
    c.renderMode = RenderMode::RenderSingle;
    c.coreRenderer.setTargetSampleCount(c.parameters.get<int>("renderer/sampling/render_samples"));
    auto renderSize = c.parameters.get<glm::ivec2>("renderer/output/render_size");
    requestResize(renderSize.x, renderSize.y);
    markRenderDirty();
}

void Core::startRenderAnim() {
    Core& c = get();
    if (c.renderMode != RenderMode::Preview) return;
    c.renderMode = RenderMode::RenderAnimation;
    c.coreRenderer.setTargetSampleCount(c.parameters.get<int>("renderer/sampling/render_samples"));
    auto renderSize = c.parameters.get<glm::ivec2>("renderer/output/render_size");
    requestResize(renderSize.x, renderSize.y);
    markRenderDirty();
    c.animation.reset(0);
}
