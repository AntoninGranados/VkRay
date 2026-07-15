#pragma once

#include <filesystem>

#include "core/structures.hpp"
#include "core/scene/scene.hpp"
#include "core/animation_handler.hpp"
#include "core/core_renderer.hpp"

class VkSmol;
class Platform;
class ParameterHandler;

class Core {
public:
    static void init(VkSmol&, Platform&, ParameterHandler&);

    static VkSmol&           getEngine();
    static Platform&         getPlatform();
    static AnimationHandler& getAnimation();
    static ParameterHandler& getParameters();
    static Scene&            getScene();
    static CoreRenderer&     getCoreRenderer();

    static RenderMode getRenderMode();
    static void       setRenderMode(RenderMode);

    static void requestAccumulationRestart();
    static void restartAccumulation();
    static bool isAccumulationRestartPending();
    static bool consumeAccumulationRestart();

    static const std::filesystem::path& getOutputPath();
    static void                         setOutputPath(std::filesystem::path);

    static void reloadShaders();
    static void startRender();
    static void startRenderAnim();

private:
    Core() = default;
    static Core& get();

    VkSmol*           engine         = nullptr;
    Platform*         platform       = nullptr;
    ParameterHandler* parameters     = nullptr;
    Scene             scene;
    AnimationHandler  animation{24 * 5, 24.0f};
    CoreRenderer      coreRenderer;
    RenderMode        renderMode     = RenderMode::Preview;
    bool              restartPending = false;
    std::filesystem::path outputPath;
};
