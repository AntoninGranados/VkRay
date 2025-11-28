#pragma once

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "./engine/engine.hpp"
#include "./camera.hpp"
#include "./scene.hpp"

typedef uint16_t index_t;

struct ScreenVertex {
    glm::vec2 position;
};

struct UBO {
    alignas(16) glm::vec3 cameraPos;
    alignas(16) glm::vec3 cameraDir;

    alignas(8) glm::vec2 screenSize;
    float aspect;
    float tanHFov;
    
    int frameCount;
    float time;

    int timeOfDay;
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
    bufferList_t uniformBuffers;

    Scene scene;
    
    size_t frame = 0;
    int frameCount = 0;

    Camera camera = Camera(glm::vec3(0.0f, 5.0f, -10.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    int timeOfDay = 2;

    void initScene();
    void drawUI(CommandBuffer commandBuffer);
    UBO fillUBO(UBO &ubo);

    void rebuildPipeline();

    glm::mat4 model = glm::mat4(1.0f);
};
