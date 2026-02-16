#pragma once

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "engine/engine.hpp"
#include "app/notification_handler.hpp"
#include "app/parameter_handler.hpp"
#include "scene/scene.hpp"

#include "app/app_context.hpp"
#include "ui_handler.hpp"
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
    Application();
    ~Application();

    void run();

    friend void InputHandler::initCallbacks(const AppContext& ctx);

private:
    VkSmol engine;
    Scene scene;
    ParameterHandler parameters;
    NotificationHandler notifications;
    UiHandler ui;
    AnimationHandler animation = AnimationHandler(128, 24.0);

    RenderState renderState;
    PathtracerUBO pathtracerUBO{};
    ScreenUBO screenUBO{};

    bool restartRender = false;
    
    AppContext ctx{ &engine, &scene, nullptr, &parameters, &notifications, &ui, &animation, &renderState, &pathtracerUBO, &screenUBO, &restartRender };
    
    InputHandler inputHandler;
    RenderHandler renderer;

    int frameCount = 0;
    bool shouldClose = false;
    
    void initParameters();
    void initScene();

    void onFrameStart(float dt);
    void clearReaderingData(RenderMode newRenderMode);
    void handleCommands();
    void fillUBOs();
    float lastTime = 0.0f;
};
