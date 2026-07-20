#include "core.hpp"
#include "core/parameters/parameter_serializer.hpp"

Core& Core::get() {
    static Core instance;
    return instance;
}

void Core::init(Platform& p, uint32_t version) {
    Core& c    = get();
    c.platform = &p;
    c.engine.init("VkRay", version, p);
    c.parameters = ParameterSerializer::load("./assets/parameters/parameters.json");
    c.coreRenderer.bindParameters();
    // TODO: move that to a meta programm
    ParameterSerializer::saveDocumentation("./docs/parameters.md");
}

void Core::terminate() {
    Core& c = get();
    c.engine.waitIdle();
    c.coreRenderer.destroy();
    c.engine.destroyGraph();
    c.scene.destroy();
    c.engine.terminate();
}

bool Core::consumeAccumulationRestart() {
    Core& c = get();
    if (!c.restartPending) return false;
    c.restartPending = false;
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

void Core::renderFrame(std::function<void(FrameContext&)> onRender) {
    Core& c = get();
    c.scene.runPreRender();
    auto frameContext = c.engine.beginFrame();
    if (!frameContext) {
        c.engine.advanceFrame();
        c.scene.runPostRender();
        return;
    }
    c.scene.runOnRender(*frameContext);
    c.coreRenderer.render(*frameContext);
    if (onRender) onRender(*frameContext);
    c.engine.advanceFrame();
    c.scene.runPostRender();
}

void Core::reloadShaders() {
    get().coreRenderer.buildPipelines();
    requestAccumulationRestart();
}

void Core::startRender() {
    Core& c = get();
    if (c.renderMode != RenderMode::Preview) return;
    c.renderMode = RenderMode::RenderSingle;
    c.coreRenderer.setTargetSampleCount(c.parameters.get<int>("renderer/sampling/render_samples"));
    auto renderSize = c.parameters.get<glm::ivec2>("renderer/output/render_size");
    requestResize(renderSize.x, renderSize.y);
    requestAccumulationRestart();
}

void Core::startRenderAnim() {
    Core& c = get();
    if (c.renderMode != RenderMode::Preview) return;
    c.renderMode = RenderMode::RenderAnimation;
    c.coreRenderer.setTargetSampleCount(c.parameters.get<int>("renderer/sampling/render_samples"));
    auto renderSize = c.parameters.get<glm::ivec2>("renderer/output/render_size");
    requestResize(renderSize.x, renderSize.y);
    requestAccumulationRestart();
    c.animation.reset(0);
}
