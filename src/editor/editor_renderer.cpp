#include "editor_renderer.hpp"

#include "imgui/imgui.h"

#include "VkSmol/engine.hpp"
#include "VkSmol/graph/builder_resource.hpp"
#include "VkSmol/graph/graph_utils.hpp"
#include "VkSmol/graph/pass/compute_pass_builder.hpp"
#include "VkSmol/graph/pass/graphics_pass_builder.hpp"
#include "VkSmol/graph/pass/present_pass_builder.hpp"
#include "VkSmol/graph/render_graph_builder.hpp"
#include "VkSmol/render/pipeline/vertex_input.hpp"

#include "editor_ui.hpp"

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

void EditorRenderer::initGraph(VkSmol& engine, RenderGraphBuilder& builder, CoreResources& coreResources) {
    editorGroupHandle = builder.addSubmissionGroup("Editor");
    uiGroupHandle = builder.addSubmissionGroup("Ui");

    swapchainImageHandle = builder.importImage(
        "SwapchainImage",
        VK_FORMAT_R32G32B32A32_SFLOAT,
        engine.getExtent().width, engine.getExtent().height, 1,
        { .usage = ImageUsageType::Undefined, .access = AccessType::None },
        { .usage = ImageUsageType::Present, .access = AccessType::Read }
    );

    vertexBufferHandle = builder.createStaticBuffer("FullscreenVertexBuffer", vertices.data(), sizeof(ScreenVertex) * vertices.size());
    indexBufferHandle  = builder.createStaticBuffer("FullscreenIndexBuffer",  indices.data(),  sizeof(index_t)     * indices.size());

    displayUBOHandle = builder.createPerFrameBuffer("DisplayUBO", sizeof(DisplayUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    GraphicsPassBuilder display = builder.addGraphicsPass("DisplayPass");
    displayPassHandle = display.getHandle();
    display.setGroup(editorGroupHandle);
    display.readImage (0, coreResources.outputImageHandle, ImageUsageType::Sampled);
    display.readBuffer(1, displayUBOHandle, BufferUsageType::Uniform);
    display.readBuffer(2, coreResources.pixelInfoBufferHandle, BufferUsageType::Storage);
    display.writeImage(swapchainImageHandle, ImageUsageType::ColorAttachment, WriteMode::Overwrite, AttachmentLoad::Clear);
    display.bindVertexBuffer(vertexBufferHandle);
    display.bindIndexBuffer(indexBufferHandle);
    VertexInput<ScreenVertex> vertexInput;
    vertexInput.addAttributeDescription(VK_FORMAT_R32G32_SFLOAT, offsetof(ScreenVertex, pos));
    displayPipelineHandle = display.setPipeline(
        vertexInput.get(),
        {
            { VK_SHADER_STAGE_VERTEX_BIT,   "./src/shaders/vert.glsl" },
            { VK_SHADER_STAGE_FRAGMENT_BIT, "./src/shaders/frag.glsl" }
        }
    );

    GraphicsPassBuilder ui = builder.addGraphicsPass("UiPass");
    uiPassHandle = ui.getHandle();
    ui.setGroup(uiGroupHandle);
    ui.writeImage(swapchainImageHandle, ImageUsageType::ColorAttachment, WriteMode::Preserve, AttachmentLoad::Load);

    PresentPassBuilder present = builder.addPresentPass("PresentPass");
    presentPassHandle = present.getHandle();
    present.setGroup(uiGroupHandle);
    present.setPresentationImage(swapchainImageHandle);

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
}

void EditorRenderer::render(AppContext& ctx, const FrameContext& frameContext) {
    VkSmol& engine = *ctx.engine;

    engine.fillBuffer(engine.getBuffer(displayUBOHandle, frameContext.currentFrame), ctx.displayUBO);

    engine.bindImage(
        swapchainImageHandle,
        engine.getSwapchainImage(frameContext.imageIndex).get(),
        engine.getSwapchainImageView(frameContext.imageIndex).get()
    );

    displayPass(ctx, frameContext);
    uiPass(ctx);
}

void EditorRenderer::displayPass(AppContext& ctx, const FrameContext& frameContext) {
    VkSmol& engine = *ctx.engine;

    CommandBuffer& commandBuffer = engine.beginRecording(editorGroupHandle);
    
    VkExtent2D extent = frameContext.extent;
    
    engine.emitBarriers(commandBuffer, displayPassHandle);
    std::vector<AttachmentInfo> attachments = engine.getColorAttachment(displayPassHandle);
    assert(attachments.size() == 1);
    engine.beginDynamicRenderer(
        commandBuffer,
        attachments[0].view, attachments[0].layout,
        attachments[0].loadOp, VK_ATTACHMENT_STORE_OP_STORE,
        {{ 0.0f, 0.0f, 0.0f, 1.0f }}
    );

    VkViewport viewport{};
    viewport.width = (float)extent.width;
    viewport.height = (float)extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{ .offset = {0, 0}, .extent = extent };

    GraphicsPipeline& diPipeline = engine.getGraphicsPipeline(displayPipelineHandle);
    engine.bindDescriptors(commandBuffer, displayPassHandle);
    diPipeline.bind(commandBuffer);
    engine.getBuffer(vertexBufferHandle).bindVertex(commandBuffer);
    engine.getBuffer(indexBufferHandle).bindIndex(commandBuffer, VK_INDEX_TYPE_UINT16);
    diPipeline.setViewport(commandBuffer, viewport);
    diPipeline.setScissor(commandBuffer, scissor);
    diPipeline.drawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()));

    engine.endDynamicRenderer(commandBuffer);
    engine.endRecording(editorGroupHandle);
}

void EditorRenderer::uiPass(AppContext& ctx) {
    VkSmol& engine = *ctx.engine;

    CommandBuffer& commandBuffer = engine.beginRecording(uiGroupHandle);

    engine.emitBarriers(commandBuffer, uiPassHandle);
    std::vector<AttachmentInfo> attachments = engine.getColorAttachment(uiPassHandle);
    assert(attachments.size() == 1);
    engine.beginDynamicRenderer(
        commandBuffer,
        attachments[0].view, attachments[0].layout,
        attachments[0].loadOp, VK_ATTACHMENT_STORE_OP_STORE,
        {{ 0.0f, 0.0f, 0.0f, 1.0f }}
    );

    ctx.ui->draw(commandBuffer, ctx);

    engine.endDynamicRenderer(commandBuffer);

    engine.emitBarriers(commandBuffer, presentPassHandle);

    // @warning this is out of place, but I need a command buffer
    engine.emitOutputBarriers(commandBuffer);

    engine.endRecording(uiGroupHandle);
}
