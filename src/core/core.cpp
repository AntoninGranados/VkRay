#include "core.hpp"

#include "VkSmol/engine.hpp"
#include "VkSmol/platform/platform.hpp"
#include "core/animation_handler.hpp"
#include "core/parameter_handler.hpp"

Core& Core::get() {
    static Core instance;
    return instance;
}

void Core::init(VkSmol& e, Platform& pl, ParameterHandler& p) {
    Core& c      = get();
    c.engine     = &e;
    c.platform   = &pl;
    c.parameters = &p;
}

VkSmol&           Core::getEngine()        { return *get().engine; }
Platform&         Core::getPlatform()      { return *get().platform; }
AnimationHandler& Core::getAnimation()     { return get().animation; }
ParameterHandler& Core::getParameters()   { return *get().parameters; }
Scene&            Core::getScene()        { return get().scene; }
CoreRenderer&     Core::getCoreRenderer() { return get().coreRenderer; }

RenderMode Core::getRenderMode()             { return get().renderMode; }
void       Core::setRenderMode(RenderMode m) { get().renderMode = m; }

void Core::restartAccumulation()    { get().restartPending = true; }
bool Core::isAccumulationPending()  { return get().restartPending; }
bool Core::consumeAccumulationRestart() {
    Core& c = get();
    if (!c.restartPending) return false;
    c.restartPending = false;
    return true;
}

const std::filesystem::path& Core::getOutputPath() { return get().outputPath; }
void Core::setOutputPath(std::filesystem::path p)  { get().outputPath = std::move(p); }

void Core::reloadShaders() {
    Core& c = get();
    c.coreRenderer.buildPipelines();
    c.restartPending = true;
}

void Core::startRender() {
    Core& c = get();
    if (c.renderMode != RenderMode::Preview) return;
    c.renderMode = RenderMode::RenderSingle;
    c.coreRenderer.setTargetSampleCount(c.parameters->getInt("pathtracer/sampling/render_samples"));
    c.restartPending = true;
}

void Core::startRenderAnim() {
    Core& c = get();
    if (c.renderMode != RenderMode::Preview) return;
    c.renderMode = RenderMode::RenderAnimation;
    c.coreRenderer.setTargetSampleCount(c.parameters->getInt("pathtracer/sampling/render_samples"));
    c.restartPending = true;
    c.animation.reset(0);
}
