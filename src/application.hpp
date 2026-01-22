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

typedef uint16_t index_t;

struct ScreenVertex {
    alignas(16) glm::vec2 position;
};

struct RaytracingUBO {
    alignas(16) glm::vec3 cameraPos;
    alignas(16) glm::vec3 cameraDir;
    float tanHFov;
    float aperture;
    float focusDepth;

    alignas(8) glm::vec2 screenSize;
    float aspect;
    float resolution;
    float prevResolution;
    
    int frameCount;
    float time;

    LightMode lightMode;

    int maxBounces;
    int samplesPerPixel;
    int importanceSampling;
    int varianceSampling;
    int varianceWarmupSamples;
    int debugView;
};

enum class DebugView : int {
    None = 0,
    Bounces,
    Normal,
    SelectionMask,
    Variance
};

struct ScreenUBO {
    int frameCount;
    float resolution;
    int debugView;
    int previewBorderEnabled;
};

struct UiState {
    bool toggled = true;
    bool capturesMouse = false;
    bool capturesKeyboard = false;
    bool toggledBeforeRender = true;
    bool middleClickWasDown = false;
};

class Application {
public:
    Application();
    ~Application();

    void run();

private:
    VkSmol engine;

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

    Scene scene;

    size_t frame = 0;
    int frameCount = 0;
    bool restartRender = false;
    bool shouldClose = false;
    
    RaytracingUBO raytracingUBO;
    ScreenUBO screenUBO;
    
    Camera camera = Camera(glm::vec3(0.0f, 0.0f, -10.0f));
    CameraHandle *previewCameraHandle = nullptr;
    float resolution = 1.0f;
    float prevResolution = resolution;
    ParameterStore parameters;

    UiState ui;
    bool renderMode = false;
    bool renderModePendingExit = false;
    double samplesPerSecEMA = 0.0;
    bool samplesPerSecInitialized = false;
    double samplesPerSecAccumTime = 0.0;
    double samplesPerSecAccumSamples = 0.0;

    bool screenshotRequested = false;
    bool screenshotPendingSave = false;
    uint32_t screenshotWidth = 0;
    uint32_t screenshotHeight = 0;
    uint64_t sampleCount = 0;

    static NotificationManager notificationManager;
    
    void initScene();
    void initParameters();

    void onFrameStart(float dt);
    void drawUI(CommandBuffer commandBuffer);
    void updateUiState();
    void drawMainUi();
    void syncPreviewCameraFromHandle();
    void drawRenderUi();
    void handleInput(float dt);
    void handleInputPreview(float dt);
    void handleInputRender(float dt);
    void fillUBOs(RaytracingUBO &raytracingUBO, ScreenUBO &screenUBO);
    float lastTime = 0.0f;

    void rebuildPipeline();

    void copyImageToScreenshotBuffer(CommandBuffer commandBuffer, Image image);
    void saveScreenshotBuffer(std::string path);
};
