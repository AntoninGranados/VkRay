#pragma once

#include <filesystem>
#include <functional>

#include "VkSmol/engine.hpp"
#include "VkSmol/frame_context.hpp"
#include "VkSmol/platform/platform.hpp"

#include "core/animation/animation_clock.hpp"
#include "core/parameters/parameters.hpp"
#include "core/parameters/parameter_serializer.hpp"
#include "core/scene/scene.hpp"
#include "core/scene_renderer.hpp"
#include "core/structures.hpp"

class Core {
public:
    static void init(Platform&, uint32_t version);
    static void terminate();

    static VkSmol&           getEngine()      { return get().engine; }
    static Platform&         getPlatform()    { return *get().platform; }
    static AnimationClock&   getAnimation()   { return get().animation; }
    static ParameterRegistry& getParameters() { return get().parameters; }
    static Scene&            getScene()       { return get().scene; }
    static SceneRenderer&    getSceneRenderer() { return get().sceneRenderer; }

    static RenderMode getRenderMode()             { return get().renderMode; }
    static void       setRenderMode(RenderMode m) { get().renderMode = m; }

    static void markDirty() { get().dirty = true; }
    static void restartAccumulation();
    static bool isDirty()     { return get().dirty; }
    static bool consumeDirty();

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

    Platform*         platform      = nullptr;
    VkSmol            engine;
    ParameterRegistry parameters;
    Scene             scene;
    AnimationClock    animation{24 * 5, 24.0f};
    SceneRenderer     sceneRenderer;
    RenderMode        renderMode     = RenderMode::Preview;
    bool              dirty          = false;
    VkExtent2D        targetExtent   = {};
    std::filesystem::path outputPath;
};
