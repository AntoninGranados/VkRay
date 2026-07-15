#include "editor_renderer.hpp"

#include "VkSmol/engine.hpp"
#include "VkSmol/graph/builder_resource.hpp"
#include "VkSmol/graph/pass/compute_pass_builder.hpp"
#include "VkSmol/graph/pass/graphics_pass_builder.hpp"
#include "VkSmol/graph/pass/present_pass_builder.hpp"
#include "VkSmol/graph/render_graph_builder.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_vulkan.h"

#include "core/core.hpp"
#include "core/parameter_handler.hpp"
#include "editor.hpp"

void EditorRenderer::initGraph(RenderGraphBuilder& builder, CoreResources& coreResources) {
    VkSmol& engine = Core::getEngine();
    editorGroupHandle = builder.addSubmissionGroup("Editor");
    uiGroupHandle     = builder.addSubmissionGroup("Ui");

    swapchainImageHandle = builder.importImage(
        "SwapchainImage",
        VK_FORMAT_R32G32B32A32_SFLOAT,
        engine.getExtent().width, engine.getExtent().height, 1,
        { .usage = ImageUsageType::Undefined, .access = AccessType::None },
        { .usage = ImageUsageType::Present,   .access = AccessType::Read }
    );

    displayImageHandle = builder.createImage(
        "DisplayImage",
        VK_FORMAT_R32G32B32A32_SFLOAT,
        engine.getExtent().width, engine.getExtent().height
    );
    debugImageHandle = builder.createImage(
        "DebugImage",
        VK_FORMAT_R32G32B32A32_SFLOAT,
        engine.getExtent().width, engine.getExtent().height
    );

    displayUBOHandle = builder.createPerFrameBuffer("DisplayUBO", sizeof(DisplayUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    debugUBOHandle   = builder.createPerFrameBuffer("DebugUBO",   sizeof(DebugUBO),   VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    // Display pass — beauty + selection edges + camera frame (compute)
    ComputePassBuilder display = builder.addComputePass("DisplayPass");
    displayPassHandle = display.getHandle();
    display.setGroup(editorGroupHandle);
    display.readImage ( 0, coreResources.outputImageHandle,        ImageUsageType::Sampled);
    display.readBuffer( 1, displayUBOHandle,                       BufferUsageType::Uniform);
    display.readBuffer( 2, coreResources.pixelInfoBufferHandle,    BufferUsageType::Storage);
    display.writeImage( 3, displayImageHandle,                     ImageUsageType::Storage);
    displayPipelineHandle = display.setPipeline("./src/shaders/editor/display.glsl");

    // Debug pass — debug view visualization (compute)
    ComputePassBuilder debug = builder.addComputePass("DebugPass");
    debugPassHandle = debug.getHandle();
    debug.setGroup(editorGroupHandle);
    debug.readBuffer( 0, debugUBOHandle,                        BufferUsageType::Uniform);
    debug.readBuffer( 1, coreResources.pixelInfoBufferHandle,   BufferUsageType::Storage);
    debug.writeImage( 2, debugImageHandle,                      ImageUsageType::Storage);
    debugPipelineHandle = debug.setPipeline("./src/shaders/editor/debug.glsl");

    // UI pass — ImGui renders over cleared swapchain; declares sampled reads to drive barriers
    GraphicsPassBuilder ui = builder.addGraphicsPass("UiPass");
    uiPassHandle = ui.getHandle();
    ui.setGroup(uiGroupHandle);
    ui.readImage(0, displayImageHandle, ImageUsageType::Sampled);
    ui.readImage(1, debugImageHandle,   ImageUsageType::Sampled);
    ui.writeImage(swapchainImageHandle, ImageUsageType::ColorAttachment, WriteMode::Overwrite, AttachmentLoad::Clear);

    PresentPassBuilder present = builder.addPresentPass("PresentPass");
    presentPassHandle = present.getHandle();
    present.setGroup(uiGroupHandle);
    present.setPresentationImage(swapchainImageHandle);

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
}

void EditorRenderer::registerImGuiTextures() {
    VkSmol& engine = Core::getEngine();

    if (displayTexId) ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)displayTexId);
    if (debugTexId)   ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)debugTexId);

    displayTexId = (ImTextureID)ImGui_ImplVulkan_AddTexture(
        engine.getSampler(displayImageHandle).get(),
        engine.getView(displayImageHandle).get(),
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
    debugTexId = (ImTextureID)ImGui_ImplVulkan_AddTexture(
        engine.getSampler(debugImageHandle).get(),
        engine.getView(debugImageHandle).get(),
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
}

void EditorRenderer::resize(uint32_t width, uint32_t height) {
    VkSmol& engine = Core::getEngine();
    renderExtent = { width, height };
    engine.resizeImage(displayImageHandle, width, height);
    engine.resizeImage(debugImageHandle,   width, height);
    registerImGuiTextures();
}

void EditorRenderer::render(const FrameContext& frameContext) {
    VkSmol& engine = Core::getEngine();

    displayUBO.previewBorderEnabled = Core::getScene().isPreviewingCamera(Core::getRenderMode()) ? 1 : 0;
    debugUBO.debugView = Core::getParameters().getEnum<int>("pathtracer/debug_view");

    engine.fillBuffer(engine.getBuffer(displayUBOHandle, frameContext.currentFrame), &displayUBO);
    engine.fillBuffer(engine.getBuffer(debugUBOHandle,   frameContext.currentFrame), &debugUBO);

    engine.bindImage(
        swapchainImageHandle,
        engine.getSwapchainImage(frameContext.imageIndex).get(),
        engine.getSwapchainImageView(frameContext.imageIndex).get()
    );

    editorPass(frameContext);
    uiPass();
}

void EditorRenderer::editorPass(const FrameContext& frameContext) {
    VkSmol& engine = Core::getEngine();
    const VkExtent2D extent = renderExtent.width > 0 ? renderExtent : frameContext.extent;
    CommandBuffer& cmd = engine.beginRecording(editorGroupHandle);

    {
        ComputePipeline& pipeline = engine.getComputePipeline(displayPipelineHandle);
        engine.emitBarriers(cmd, displayPassHandle);
        engine.bindDescriptors(cmd, displayPassHandle);
        pipeline.bind(cmd);
        pipeline.dispatch(cmd, (extent.width + 7) / 8, (extent.height + 7) / 8);
    }
    {
        ComputePipeline& pipeline = engine.getComputePipeline(debugPipelineHandle);
        engine.emitBarriers(cmd, debugPassHandle);
        engine.bindDescriptors(cmd, debugPassHandle);
        pipeline.bind(cmd);
        pipeline.dispatch(cmd, (extent.width + 7) / 8, (extent.height + 7) / 8);
    }

    engine.endRecording(editorGroupHandle);
}

void EditorRenderer::uiPass() {
    VkSmol& engine = Core::getEngine();
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

    Editor::getUi().draw(commandBuffer);

    engine.endDynamicRenderer(commandBuffer);

    engine.emitBarriers(commandBuffer, presentPassHandle);

    // @warning this is out of place, but I need a command buffer
    engine.emitOutputBarriers(commandBuffer);

    engine.endRecording(uiGroupHandle);
}
