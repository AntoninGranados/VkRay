#pragma once

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "./engine/engine.hpp"
#include "./camera.hpp"
#include "./notification_system.hpp"
#include "./parameter_system.hpp"
#include "./scene/scene.hpp"
#include "./scene/camera_handle.hpp"
#include "./scene/scene_preset.hpp"
#include "./scene/object/object.hpp"

#include "./app_context.hpp"
#include "./ui_system.hpp"
#include "./render_system.hpp"

enum class DebugView : int {
    None = 0,
    Bounces,
    Normal,
    SelectionMask,
    Variance
};

class Application {
public:
    Application();
    ~Application();

    void run();

private:
    VkSmol engine;
    Scene scene;
    Camera camera = Camera(glm::vec3(0.0f, 0.0f, -10.0f));
    ParameterSystem parameters;
    NotificationSystem notifications;
    UiSystem ui;

    RenderState renderState;
    PathtracerUBO pathtracerUBO{};
    ScreenUBO screenUBO{};

    bool restartRender = false;
    
    AppContext ctx{ &engine, &scene, &camera, &parameters, &notifications, &ui, &renderState, &pathtracerUBO, &screenUBO, &restartRender };
    
    RenderSystem renderer;

    int frameCount = 0;
    bool shouldClose = false;
    
    CameraHandle *previewCameraHandle = nullptr;
    
    void initParameters();
    void initScene();

    void onFrameStart(float dt);
    void syncPreviewCameraFromHandle();
    void handleInput(float dt);
    void handleInputPreview(float dt);
    void handleInputRender(float dt);
    void fillUBOs();
    float lastTime = 0.0f;
};
