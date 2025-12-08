#pragma once

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "./engine/engine.hpp"
#include "./camera.hpp"
#include "./notification.hpp"
#include "./scene/scene.hpp"
#include "./scene/object/object.hpp"

typedef uint16_t index_t;

enum LightMode : int {
    Day,
    Sunset,
    Night,
    Empty,
};

struct ScreenVertex {
    glm::vec2 position;
};

struct RaytracingUBO {
    alignas(16) glm::vec3 cameraPos;
    alignas(16) glm::vec3 cameraDir;

    alignas(8) glm::vec2 screenSize;
    float aspect;
    float tanHFov;
    
    int frameCount;
    float time;

    LightMode lightMode;

    int maxBounces;
    int samplesPerPixel;
};

struct ScreenUBO {
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

    Scene scene;
    
    size_t frame = 0;
    int frameCount = 0;

    Camera camera = Camera(glm::vec3(0.0f, 0.0f, -10.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    LightMode lightMode = LightMode::Empty;
    int maxBounces = 5;
    int samplesPerPixel = 1;

    bool uiCapturesMouse = false;
    bool uiCapturesKeyboard = false;
    bool uiToggled = true;

    NotificationManager notificationManager;

    void initScene();
    void drawUI(CommandBuffer commandBuffer);
    void fillUBOs(RaytracingUBO &raytracingUBO, ScreenUBO &screenUBO);
    float lastTime = 0.0f;

    void rebuildPipeline();
};
