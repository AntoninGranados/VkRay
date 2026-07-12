#pragma once

#include <optional>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "VkSmol/engine.hpp"
#include "VkSmol/platform/platform.hpp"

#include "core/parameter_handler.hpp"
#include "editor/editor_renderer.hpp"
#include "core/scene/scene.hpp"
#include "core/scene/scene_serializer.hpp"

#include "app_context.hpp"
#include "editor/editor_ui.hpp"
#include "core/animation_handler.hpp"
#include "core/core_renderer.hpp"
#include "offline/job_queue.hpp"

#include "editor/input_handler.hpp"

enum class DebugView : int {
    None = 0,
    PositionW,
    Position,
    NormalW,
    Normal,
    Albedo,
    Roughness,
    MatType,
    Bounces,
    HitChecks,
    Variance,
    SelectionMask,
    SkyMask,
};

class Application {
public:
    explicit Application(Platform& platform);
    ~Application();

    void run();
    void runJobs(JobQueue& queue);

    friend void InputHandler::initCallbacks(const AppContext& ctx);

private:
    Platform& platform;

    VkSmol engine;
    Scene scene;
    ParameterHandler parameters;
    std::optional<EditorUi> ui;
    AnimationHandler animation = AnimationHandler(24.0f * 5, 24.0f);

    bool restartRender = false;

    std::optional<InputHandler> inputHandler;
    CoreRenderer coreRenderer;
    EditorRenderer editorRenderer;

    AppContext ctx{
        .engine        = &engine,
        .scene         = &scene,
        .parameters    = &parameters,
        .animation     = &animation,
        .restartRender = &restartRender,
        .platform      = &platform
    };

    uint64_t lastSwapchainGeneration = 0;
    void initParameters();
    void initScene(const std::string& sceneFile = "assets/scenes/default.json");

    void onFrameStart(float dt);
    void clearRenderingData(RenderMode newRenderMode);
    void fillUBOs();
};
