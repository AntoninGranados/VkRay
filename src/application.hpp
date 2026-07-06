#pragma once

#include <optional>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "VkSmol/engine.hpp"
#include "VkSmol/platform/platform.hpp"

#include "app/parameter_handler.hpp"
#include "scene/scene.hpp"
#include "scene/scene_serializer.hpp"

#include "app/app_context.hpp"
#include "editor/editor_ui.hpp"
#include "app/animation_handler.hpp"
#include "render/job_queue.hpp"
#include "render/render_handler.hpp"

#include "app/input_handler.hpp"

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

    RenderState renderState;
    PathtracerUBO pathtracerUBO{};
    CompositingUBO compositingUBO{};
    DisplayUBO displayUBO{};
    bool restartRender = false;

    std::optional<InputHandler> inputHandler;
    RenderHandler renderer;

    AppContext ctx{
        .engine         = &engine,
        .scene          = &scene,
        .parameters     = &parameters,
        .animation      = &animation,
        .renderState    = &renderState,
        .pathtracerUBO  = &pathtracerUBO,
        .compositingUBO = &compositingUBO,
        .displayUBO     = &displayUBO,
        .restartRender  = &restartRender,
        .platform       = &platform
    };

    int frameCount = 0;
    void initParameters();
    void initScene(const std::string& sceneFile = "assets/scenes/default.json");

    void onFrameStart(float dt);
    void clearRenderingData(RenderMode newRenderMode);
    void fillUBOs();
    void fillJobUBOs(uint32_t sampleIndex);
    float lastTime = 0.0f;
};
