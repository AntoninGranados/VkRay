#pragma once

#include <filesystem>
#include <functional>

#include "VkSmol/engine.hpp"
#include "VkSmol/frame_context.hpp"
#include "VkSmol/platform/platform.hpp"

#include "core/animation_handler.hpp"
#include "core/core_renderer.hpp"
#include "core/parameters/parameters.hpp"
#include "core/parameters/parameter_serializer.hpp"
#include "core/scene/scene.hpp"
#include "core/structures.hpp"

class Core {
public:
    static void init(Platform&, uint32_t version);
    static void terminate();

    static VkSmol&           getEngine()      { return get().engine; }
    static Platform&         getPlatform()    { return *get().platform; }
    static AnimationHandler& getAnimation()   { return get().animation; }
    static ParameterHandler& getParameters()  { return get().parameters; }
    static Scene&            getScene()       { return get().scene; }
    static CoreRenderer&     getCoreRenderer(){ return get().coreRenderer; }

    static RenderMode getRenderMode()             { return get().renderMode; }
    static void       setRenderMode(RenderMode m) { get().renderMode = m; }

    static void requestAccumulationRestart() { get().restartPending = true; }
    static void restartAccumulation()        { get().coreRenderer.restartAccumulation(); }
    static bool isAccumulationRestartPending()  { return get().restartPending; }
    static bool consumeAccumulationRestart();

    static void resize(int width, int height);
    static void requestResize(int width, int height) { get().targetExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) }; }
    static bool consumeResize();

    static void renderFrame(std::function<void(FrameContext&)> onRender = {});

    static const std::filesystem::path& getOutputPath() { return get().outputPath; }
    static void setOutputPath(std::filesystem::path p) { get().outputPath = std::move(p); }

    static void reloadShaders();
    static void startRender();
    static void startRenderAnim();

private:
    Core() = default;
    static Core& get();

    Platform*        platform      = nullptr;
    VkSmol           engine;
    ParameterHandler parameters;
    Scene            scene;
    AnimationHandler animation{24 * 5, 24.0f};
    CoreRenderer     coreRenderer;
    RenderMode       renderMode     = RenderMode::Preview;
    bool             restartPending = false;
    VkExtent2D       targetExtent   = {};
    std::filesystem::path outputPath;
};
