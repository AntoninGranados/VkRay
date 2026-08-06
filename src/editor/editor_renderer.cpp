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
#include "core/parameters/parameters.hpp"
#include "editor.hpp"
#include "editor/ui_utils.hpp"

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
    outputImageHandle = coreResources.outputImageHandle;

    debugUBOHandle = builder.createPerFrameBuffer("DebugUBO", sizeof(DebugUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    // Display pass — beauty + selection edges + camera frame (compute)
    ComputePassBuilder display = builder.addComputePass("DisplayPass");
    displayPassHandle = display.getHandle();
    display.setGroup(editorGroupHandle);
    display.readImage ( 0, coreResources.outputImageHandle, ImageUsageType::Sampled);
    display.readBuffer( 1, coreResources.pixelInfoBufferHandle, BufferUsageType::Storage);
    display.writeImage( 2, displayImageHandle, ImageUsageType::Storage);
    display.setPipeline("./src/shaders/editor/display.glsl");
    displayTimestamp = display.setTimestamp();

    // Debug pass — debug view visualization (compute)
    ComputePassBuilder debug = builder.addComputePass("DebugPass");
    debugPassHandle = debug.getHandle();
    debug.setGroup(editorGroupHandle);
    debug.readBuffer( 0, debugUBOHandle, BufferUsageType::Uniform);
    debug.readBuffer( 1, coreResources.pixelInfoBufferHandle, BufferUsageType::Storage);
    debug.writeImage( 2, debugImageHandle, ImageUsageType::Storage);
    debug.setPipeline("./src/shaders/editor/debug.glsl");
    debugTimestamp = debug.setTimestamp();

    // UI pass — ImGui renders over cleared swapchain; declares sampled reads to drive barriers
    GraphicsPassBuilder ui = builder.addGraphicsPass("UiPass");
    uiPassHandle = ui.getHandle();
    ui.setGroup(uiGroupHandle);
    ui.readImage(0, displayImageHandle, ImageUsageType::Sampled);
    ui.readImage(1, debugImageHandle,   ImageUsageType::Sampled);
    ui.readImage(2, coreResources.outputImageHandle, ImageUsageType::Sampled);
    ui.writeImage(swapchainImageHandle, ImageUsageType::ColorAttachment, WriteMode::Overwrite, AttachmentLoad::Clear);
    uiTimestamp = ui.setTimestamp();

    PresentPassBuilder present = builder.addPresentPass("PresentPass");
    presentPassHandle = present.getHandle();
    present.setGroup(uiGroupHandle);
    present.setPresentationImage(swapchainImageHandle);

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
}

void EditorRenderer::registerImGuiTextures() {
    VkSmol& engine = Core::getEngine();

    if (outputTexId)  ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)outputTexId);
    if (displayTexId) ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)displayTexId);
    if (debugTexId)   ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)debugTexId);

    outputTexId = (ImTextureID)ImGui_ImplVulkan_AddTexture(
        engine.getSampler(outputImageHandle).get(),
        engine.getView(outputImageHandle).get(),
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
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
    const VkExtent2D extent = renderExtent.width > 0 ? renderExtent : frameContext.extent;

    debugUBO.debugView = Core::getParameters().get<DebugView>("renderer/debug_view");

    engine.fillBuffer(engine.getBuffer(debugUBOHandle, frameContext.currentFrame), &debugUBO);

    engine.bindImage(
        swapchainImageHandle,
        engine.getSwapchainImage(frameContext.imageIndex).get(),
        engine.getSwapchainImageView(frameContext.imageIndex).get()
    );

    {
        CommandBuffer& cmd = engine.beginRecording(editorGroupHandle);
     
        engine.dispatch(cmd, displayPassHandle, (extent.width + 7) / 8, (extent.height + 7) / 8);
        engine.dispatch(cmd, debugPassHandle,   (extent.width + 7) / 8, (extent.height + 7) / 8);
     
        engine.endRecording(editorGroupHandle);
    }
    
    {
        CommandBuffer& commandBuffer = engine.beginRecording(uiGroupHandle);
     
        engine.beginGraphics(commandBuffer, uiPassHandle, &ui::kDraculaBg.x);
        Editor::getUi().draw(commandBuffer);
        engine.endGraphics(commandBuffer, uiPassHandle);
    
        engine.emitBarriers(commandBuffer, presentPassHandle);
    
        // @warning this is out of place, but I need a command buffer
        engine.emitOutputBarriers(commandBuffer);
    
        engine.endRecording(uiGroupHandle);
    }
}
