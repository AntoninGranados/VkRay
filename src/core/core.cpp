#include "core.hpp"

Core& Core::get() {
    static Core instance;
    return instance;
}

void Core::init(Platform& p, uint32_t version) {
    Core& c    = get();
    c.platform = &p;
    c.engine.init("VkRay", version, p);
    c.parameters = ParameterHandler::fromFile();
    c.coreRenderer.bindParameters();
    c.parameters.saveDocumentation();
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
    c.coreRenderer.setTargetSampleCount(c.parameters.getInt("render/render_samples"));
    requestAccumulationRestart();
}

void Core::startRenderAnim() {
    Core& c = get();
    if (c.renderMode != RenderMode::Preview) return;
    c.renderMode = RenderMode::RenderAnimation;
    c.coreRenderer.setTargetSampleCount(c.parameters.getInt("render/render_samples"));
    requestAccumulationRestart();
    c.animation.reset(0);
}
