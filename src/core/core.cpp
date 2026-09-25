#include "core.hpp"

#include "core/ecs/components/component_serializer.hpp"
#include "core/fields/parameter_serializer.hpp"
#include "core/render/camera_lens_table.hpp"
#include "core/render/material_table.hpp"

Core& Core::get() {
    static Core instance;
    return instance;
}

void Core::init(Platform& p, uint32_t version) {
    Core& c    = get();
    c.platform = &p;
    c.engine.init("VkRay", version, p);
    c.parameters = ParameterSerializer::load("./src/config/parameters.json");
    c.coreRenderer.bindParameters();
    // TODO: move that to a meta programm (compile time)
    ParameterSerializer::saveDocumentation("./docs/parameters.md");
    ComponentSerializer::saveDocumentation("./docs/components.md");
}

void Core::terminate() {
    Core& c = get();
    c.engine.waitIdle();
    c.coreRenderer.destroy();
    c.engine.destroyGraph();
    c.coreRenderer.getScene().destroy();
    c.engine.terminate();
}

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

void Core::reloadPipelinesIfDirty(Core& c) {
    if (!c.pipelinesDirty) return;
    c.pipelinesDirty = false;
    reloadShaders();
}

void Core::rebuildGraphIfRequested(Core& c) {
    if (!c.graphRebuildRequested) return;
    c.graphRebuildRequested = false;
    if (!c.graphBuilder) return;

    c.engine.waitIdle();
    c.coreRenderer.destroy();
    c.engine.destroyGraph();
    c.graphBuilder();
    c.restartAccumulation();
}

void Core::renderFrame(std::function<void(FrameContext&)> onRender) {
    Core& c = get();
    Scene& scene = c.coreRenderer.getScene();
    c.fileWatcher.poll();

    rebuildGraphIfRequested(c);

    updateAnimationDirty(c);
    scene.runPreRender();

    auto frameContext = c.engine.beginFrame();
    if (!frameContext) {
        c.engine.advanceFrame();
        return;
    }

    scene.runOnRender(*frameContext);
    reloadPipelinesIfDirty(c);

    c.coreRenderer.render(*frameContext);
    if (onRender) onRender(*frameContext);
    c.engine.advanceFrame();
}

void Core::reloadShaders() {
    Core& c = get();
    MaterialTable::generateDispatch();
    CameraLensTable::generateDispatch();
    c.coreRenderer.buildPipelines();
    markRenderDirty();
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
