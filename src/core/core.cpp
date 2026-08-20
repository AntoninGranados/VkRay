#include "core.hpp"

#include "core/ecs/components/component_serializer.hpp"
#include "core/parameters/parameter_serializer.hpp"

Core& Core::get() {
    static Core instance;
    return instance;
}

void Core::init(Platform& p, uint32_t version) {
    Core& c    = get();
    c.platform = &p;
    c.engine.init("VkRay", version, p);
    c.parameters = ParameterSerializer::load("./src/config/parameters.json");
    c.sceneRenderer.bindParameters();
    // TODO: move that to a meta programm
    ParameterSerializer::saveDocumentation("./docs/parameters.md");
    ComponentSerializer::saveDocumentation("./docs/components.md");
}

void Core::terminate() {
    Core& c = get();
    c.engine.waitIdle();
    c.sceneRenderer.destroy();
    c.engine.destroyGraph();
    c.scene.destroy();
    c.engine.terminate();
}

void Core::restartAccumulation() {
    Core& c = get();
    c.sceneRenderer.restartAccumulation();
    if (c.renderMode == RenderMode::Preview) {
        const int n = c.parameters.get<int>("renderer/viewport/max_samples");
        c.sceneRenderer.setTargetSampleCount(n > 0 ? n : SampleAccumulator::kUnboundedSamples);
    }
}

bool Core::consumeDirty() {
    Core& c = get();
    if (!c.dirty) return false;
    c.dirty = false;
    c.restartAccumulation();
    return true;
}

void Core::resize(int width, int height) {
    get().sceneRenderer.resize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
}

bool Core::consumeResize() {
    Core& c = get();
    if (c.targetExtent.width == 0) return false;
    VkExtent2D current = c.sceneRenderer.getRenderExtent();
    if (c.targetExtent.width == current.width && c.targetExtent.height == current.height) return false;
    resize(c.targetExtent.width, c.targetExtent.height);
    return true;
}

void Core::renderFrame(std::function<void(FrameContext&)> onRender) {
    Core& c = get();
    const float jitterRange = c.renderMode != RenderMode::Preview ? c.scene.getCamera().getShutterSpeed() : 0.0f;
    if (c.animation.sample(jitterRange)) markDirty();
    if (!c.animation.isPaused()) markDirty();
    consumeDirty();
    c.scene.runPreRender();
    auto frameContext = c.engine.beginFrame();
    if (!frameContext) {
        c.engine.advanceFrame();
        return;
    }
    c.scene.runOnRender(*frameContext);
    c.sceneRenderer.render(*frameContext, c.scene.getCamera());
    if (onRender) onRender(*frameContext);
    c.engine.advanceFrame();
}

void Core::reloadShaders() {
    get().sceneRenderer.buildPipelines();
    markDirty();
}

void Core::startRender() {
    Core& c = get();
    if (c.renderMode != RenderMode::Preview) return;
    c.renderMode = RenderMode::RenderSingle;
    c.sceneRenderer.setTargetSampleCount(c.parameters.get<int>("renderer/sampling/render_samples"));
    auto renderSize = c.parameters.get<glm::ivec2>("renderer/output/render_size");
    requestResize(renderSize.x, renderSize.y);
    markDirty();
}

void Core::startRenderAnim() {
    Core& c = get();
    if (c.renderMode != RenderMode::Preview) return;
    c.renderMode = RenderMode::RenderAnimation;
    c.sceneRenderer.setTargetSampleCount(c.parameters.get<int>("renderer/sampling/render_samples"));
    auto renderSize = c.parameters.get<glm::ivec2>("renderer/output/render_size");
    requestResize(renderSize.x, renderSize.y);
    markDirty();
    c.animation.reset(0);
}
