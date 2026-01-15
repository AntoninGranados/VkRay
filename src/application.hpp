#pragma once

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "./engine/engine.hpp"
#include "./camera.hpp"
#include "./notification.hpp"
#include "./scene/scene.hpp"
#include "./scene/scene_preset.hpp"
#include "./scene/object/object.hpp"

typedef uint16_t index_t;

struct ScreenVertex {
    glm::vec2 position;
};

struct RaytracingUBO {
    alignas(16) glm::vec3 cameraPos;
    alignas(16) glm::vec3 cameraDir;
    float tanHFov;
    float aperture;
    float focusDepth;

    alignas(8) glm::vec2 screenSize;
    float aspect;
    float lowResolutionScale;
    
    int frameCount;
    float time;

    LightMode lightMode;

    int maxBounces;
    int samplesPerPixel;
    int importanceSampling;
};

struct ScreenUBO {
    int frameCount;
    float lowResolutionScale;
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

    Scene scene;

    
    size_t frame = 0;
    int frameCount = 0;
    bool restartRender = false;
    bool shouldClose = false;
    
    RaytracingUBO raytracingUBO;
    ScreenUBO screenUBO;
    
    Camera camera = Camera(glm::vec3(0.0f, 0.0f, -10.0f));
    LightMode lightMode = LightMode::Empty;
    int maxBounces = 4;
    int samplesPerPixel = 1;
    float lowResolutionScale = 8.0f;    // TODO: change the resolution dynamically (no computation in the shader so this is always used and not only when moving)
    bool importanceSampling = true;

    bool uiCapturesMouse = false;
    bool uiCapturesKeyboard = false;
    bool uiToggled = true;
    bool middleClickWasDown = false;

    bool screenshotRequested = false;
    bool screenshotPendingSave = false;
    uint32_t screenshotWidth = 0;
    uint32_t screenshotHeight = 0;

    static NotificationManager notificationManager;
    
    void initScene();

    void onFrameStart(float dt);
    void drawUI(CommandBuffer commandBuffer);
    void fillUBOs(RaytracingUBO &raytracingUBO, ScreenUBO &screenUBO);
    float lastTime = 0.0f;

    void rebuildPipeline();

    void copyImageToScreenshotBuffer(CommandBuffer commandBuffer, Image image);
    void saveScreenshotBuffer(std::string path);
};
