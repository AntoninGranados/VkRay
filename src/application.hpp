#pragma once

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "./engine/engine.hpp"
#include "./notification_handler.hpp"
#include "./parameter_handler.hpp"
#include "./scene/scene.hpp"

#include "./app_context.hpp"
#include "./ui_handler.hpp"
#include "./animation_handler.hpp"
#include "./render_handler.hpp"

enum class DebugView : int {
    None = 0,
    Bounces,
    Normal,
    SelectionMask,
    Variance,
    HitChecks
};

class Application {
public:
    Application();
    ~Application();

    void run();

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
    
    RenderHandler renderer;

    int frameCount = 0;
    bool shouldClose = false;
    
    void initParameters();
    void initScene();

    void onFrameStart(float dt);
    void handleInput(float dt);
    void handleInputPreview(float dt);
    void handleInputRender(float dt);
    void fillUBOs();
    float lastTime = 0.0f;
};
