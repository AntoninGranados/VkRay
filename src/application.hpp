#pragma once

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "./engine/engine.hpp"
#include "./camera.hpp"
#include "./notification.hpp"
#include "./parameters.hpp"
#include "./scene/scene.hpp"
#include "./scene/camera_handle.hpp"
#include "./scene/scene_preset.hpp"
#include "./scene/object/object.hpp"

#include "./app_context.hpp"
#include "./ui_system.hpp"

typedef uint16_t index_t;

struct ScreenVertex {
    alignas(16) glm::vec2 position;
};

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
    ParameterStore parameters;
    NotificationManager notifications;
    RendererState renderer;
    PathtracerUBO pathtracerUBO{};
    ScreenUBO screenUBO{};

    bool restartRender = false;
    
    AppContext ctx{ &engine, &scene, &camera, &parameters, &notifications, &renderer, &pathtracerUBO, &screenUBO, &restartRender };
    
    UiSystem ui;

    Image images[2];
    ImageView imageViews[2];
    Sampler samplers[2];
    
    DescriptorSetLayout setLayout, screenSetLayout;
    descriptorSetList_t descriptorSets[2], screenDescriptorSets[2];
    GraphicsPipeline pipeline, screenPipeline;
    
    Buffer vertexBuffer, indexBuffer;
    bufferList_t raytracingUniformBuffers, screenUniformBuffers;
    Buffer screenshotBuffer;
    bufferList_t pixelInfoBuffers;

    size_t frame = 0;
    int frameCount = 0;
    bool shouldClose = false;
    
    CameraHandle *previewCameraHandle = nullptr;

    bool screenshotRequested = false;
    bool screenshotPendingSave = false;
    uint32_t screenshotWidth = 0;
    uint32_t screenshotHeight = 0;
    
    void initParameters();
    void initScene();

    void onFrameStart(float dt);
    void syncPreviewCameraFromHandle();
    void handleInput(float dt);
    void handleInputPreview(float dt);
    void handleInputRender(float dt);
    void fillUBOs();
    float lastTime = 0.0f;

    void rebuildPipeline();

    void copyImageToScreenshotBuffer(CommandBuffer commandBuffer, Image image);
    void saveScreenshotBuffer(std::string path);
};
