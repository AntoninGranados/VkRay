#pragma once

#include "./app_context.hpp"
#include "./engine/engine.hpp"
#include "./scene/scene.hpp"
#include "./camera.hpp"
#include "./notification_system.hpp"
#include "./ui_system.hpp"

#include "./imgui/imgui.h"
#include "./imgui/ImGuizmo.h"

typedef uint16_t index_t;

struct ScreenVertex {
    alignas(16) glm::vec2 pos;
};

const std::vector<ScreenVertex> vertices = {
    { .pos = {  1.0f, 1.0f } },
    { .pos = {  1.0f,-1.0f } },
    { .pos = { -1.0f,-1.0f } },
    { .pos = { -1.0f, 1.0f } }
};

const std::vector<index_t> indices = {
    0, 1, 2, 2, 3, 0
};

struct RenderOutput {
    bool requested   = false;
    bool pendingSave = false;
    uint32_t width   = 0;
    uint32_t height  = 0;
};

class RenderSystem {
public:
    void init(AppContext& ctx);
    void destroy(AppContext& ctx);
    void buildPipeline(AppContext& ctx);

    void render(AppContext& ctx);

private:
    Image images[2];
    ImageView imageViews[2];
    Sampler samplers[2];
    
    DescriptorSetLayout setLayout, screenSetLayout;
    descriptorSetList_t descriptorSets[2], screenDescriptorSets[2];
    GraphicsPipeline pipeline, screenPipeline;
    
    Buffer vertexBuffer, indexBuffer;
    bufferList_t pathtracingUniformBuffers, screenUniformBuffers;
    Buffer screenshotBuffer;
    bufferList_t pixelInfoBuffers;

    RenderOutput renderOutput;

    size_t frame = 0;

    void renderMain(AppContext& ctx);
    void renderUi(AppContext& ctx);

    void copyImageToScreenshotBuffer(AppContext& ctx, CommandBuffer& commandBuffer, Image& image);
    void saveScreenshotBuffer(AppContext& ctx, std::string path);
};
