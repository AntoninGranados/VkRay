#pragma once

#include <optional>
#include <string>
#include <utility>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "VkSmol/engine.hpp"
#include "VkSmol/platform/platform.hpp"

#include "app/notification_handler.hpp"
#include "app/parameter_handler.hpp"
#include "scene/scene.hpp"
#include "scene/scene_serializer.hpp"

#include "app/app_context.hpp"
#include "editor/editor_ui.hpp"
#include "app/animation_handler.hpp"
#include "render/render_handler.hpp"

#include "app/input_handler.hpp"

enum class DebugView : int {
    None = 0,
    PositionW = 1,
    Position = 2,
    NormalW = 3,
    Normal = 4,
    Albedo = 5,
    Roughness = 6,
    MatType = 7,
    Bounces = 8,
    HitChecks = 9,
    Variance = 10,
    SelectionMask = 11,
    SkyMask = 12,
};

class Application {
public:
    explicit Application(Platform& platform);
    ~Application();

    void run();
    void runHeadless(const std::filesystem::path& sceneFile, uint32_t targetSamples, const std::filesystem::path& outputPath);

    friend void InputHandler::initCallbacks(const AppContext& ctx);

private:
    Platform& platform;

    VkSmol engine;
    Scene scene;
    ParameterHandler parameters;
    NotificationHandler notifications;
    std::optional<EditorUi> ui;
    AnimationHandler animation = AnimationHandler(24.0f * 5, 24.0f);

    RenderState renderState;
    PathtracerUBO pathtracerUBO{};
    DisplayUBO displayUBO{};
    bool restartRender = false;

    AppContext ctx{
        .engine        = &engine,
        .scene         = &scene,
        .parameters    = &parameters,
        .notifications = &notifications,
        .animation     = &animation,
        .renderer      = &renderer,
        .renderState   = &renderState,
        .pathtracerUBO = &pathtracerUBO,
        .displayUBO    = &displayUBO,
        .restartRender = &restartRender,
        .platform      = &platform
    };

    std::optional<InputHandler> inputHandler;
    RenderHandler renderer;

    int frameCount = 0;
    bool shouldClose = false;

    void initParameters();
    void initScene(const std::string& sceneFile = "res/scenes/default.json");

    void onFrameStart(float dt);
    void clearRenderingData(RenderMode newRenderMode);
    void handleCommands();
    void fillUBOs();
    void fillHeadlessUBOs(int sampleIndex, uint32_t targetSamples, LightMode lightMode);
    float lastTime = 0.0f;
};
