#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <optional>
#include <string>

#include "VkSmol/engine.hpp"
#include "VkSmol/platform/platform.hpp"
#include "app/notification_handler.hpp"
#include "app/parameter_handler.hpp"
#include "scene/scene.hpp"
#include "scene/scene_preset.hpp"

#include "app/app_context.hpp"
#include "editor/editor_ui.hpp"
#include "app/animation_handler.hpp"
#include "render/render_handler.hpp"

#include "app/input_handler.hpp"

enum class DebugView : int {
    None = 0,
    Bounces = 1,
    Normal = 2,
    Position = 3,
    Diffuse = 4,
    SelectionMask = 5,
    Variance = 6,
    HitChecks = 7
};

class Application {
public:
    explicit Application(Platform& platform);
    ~Application();

    void run();
    void runHeadless(ScenePreset preset, uint32_t targetSamples, const std::string& outputPath);

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
    ScreenUBO screenUBO{};

    bool restartRender = false;

    AppContext ctx{ &engine, &scene, nullptr, &parameters, &notifications, nullptr, &animation, &renderState, &pathtracerUBO, &screenUBO, &restartRender, &platform };

    std::optional<InputHandler> inputHandler;
    RenderHandler renderer;

    int frameCount = 0;
    bool shouldClose = false;

    void initParameters();
    void initScene(ScenePreset preset = ScenePreset::Empty);

    void onFrameStart(float dt);
    void clearRenderingData(RenderMode newRenderMode);
    void handleCommands();
    void fillUBOs();
    void fillHeadlessUBOs(int sampleIndex, uint32_t targetSamples, LightMode lightMode);
    float lastTime = 0.0f;
};
