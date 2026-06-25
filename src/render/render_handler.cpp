#include "render_handler.hpp"

#include <iostream>
#include <cmath>
#include <cstring>
#include <vector>

#include <stb_image/stb_image_write.h>
#include "./imgui/imgui.h"

#include "VkSmol/engine.hpp"
#include "VkSmol/render/pipeline/vertex_input.hpp"
#include "VkSmol/graph/graph_utils.hpp"
#include "VkSmol/graph/pass/compute_pass_builder.hpp"
#include "VkSmol/graph/pass/graphics_pass_builder.hpp"
#include "VkSmol/graph/pass/present_pass_builder.hpp"
#include "VkSmol/graph/pass/transfer_pass_builder.hpp"
#include "VkSmol/graph/render_graph_builder.hpp"
#include "VkSmol/graph/builder_resource.hpp"

#include "app/app_context.hpp"
#include "app/notification_handler.hpp"
#include "app/parameter_handler.hpp"
#include "VkSmol/image/image.hpp"
#include "scene/scene.hpp"
#include "editor/editor_ui.hpp"

void RenderHandler::init(AppContext& ctx) {
    VkSmol& engine = *ctx.engine;


    RenderGraphBuilder builder;
    mainGroupHandle = builder.addSubmissionGroup("Main");
    if (!engine.isHeadless())
        uiGroupHandle = builder.addSubmissionGroup("Ui");

    if (!engine.isHeadless()) {
        swapchainImageHandle = builder.importImage(
            "SwapchainImage",
            VK_FORMAT_R32G32B32A32_SFLOAT,
            engine.getExtent().width, engine.getExtent().height, 1,
            { .usage = ImageUsageType::Undefined, .access = AccessType::None },
            { .usage = ImageUsageType::Present, .access = AccessType::Read }
        );
    }

    previousPathtracingImageHandle = builder.createImage(
        "PreviousPathtracingImage",
        VK_FORMAT_R32G32B32A32_SFLOAT,
        engine.getExtent().width, engine.getExtent().height, 1,
        { .usage = ImageUsageType::Sampled, .access = AccessType::Read },
        { .usage = ImageUsageType::Sampled, .access = AccessType::Read },
        VK_IMAGE_USAGE_STORAGE_BIT
    );
    currentPathtracingImageHandle = builder.createImage(
        "CurrentPathtracingImage",
        VK_FORMAT_R32G32B32A32_SFLOAT,
        engine.getExtent().width, engine.getExtent().height
    );
    outputImageHandle = builder.createImage(
        "OutputImage",
        VK_FORMAT_R32G32B32A32_SFLOAT,
        engine.getExtent().width, engine.getExtent().height
    );

    if (!engine.isHeadless()) {
        vertexBufferHandle = builder.createStaticBuffer("FullscreenVertexBuffer", vertices.data(), sizeof(ScreenVertex) * vertices.size());
        indexBufferHandle  = builder.createStaticBuffer("FullscreenIndexBuffer",  indices.data(),  sizeof(index_t)     * indices.size());
    }

    VkExtent2D ext        = engine.getExtent();
    pathtracingUBOHandle  = builder.createPerFrameBuffer("PathtracingUBO", sizeof(PathtracerUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    displayUBOHandle      = builder.createPerFrameBuffer("DisplayUBO",     sizeof(ScreenUBO),     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    pixelInfoBufferHandle = builder.createBuffer(
        "PixelInfoBuffer",
        static_cast<size_t>(ext.width) * ext.height * sizeof(PixelInfo),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
    );

    // Scene buffers must be created before passes so binding slots can be declared
    SceneGpuBuffers sceneHandles = {};
    sceneHandles.sphere   = { builder.createPerFrameBuffer("SceneSphereBuffer",   16 * sizeof(GpuSphere),   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    sceneHandles.plane    = { builder.createPerFrameBuffer("ScenePlaneBuffer",    16 * sizeof(GpuPlane),    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    sceneHandles.box      = { builder.createPerFrameBuffer("SceneBoxBuffer",      16 * sizeof(GpuBox),      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    sceneHandles.vertex   = { builder.createPerFrameBuffer("SceneVertexBuffer",   16 * sizeof(Vertex),      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    sceneHandles.index    = { builder.createPerFrameBuffer("SceneIndexBuffer",    16 * sizeof(uint32_t),    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    sceneHandles.bvh      = { builder.createPerFrameBuffer("SceneBvhBuffer",      16 * sizeof(GpuBvhNode),  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    sceneHandles.mesh     = { builder.createPerFrameBuffer("SceneMeshBuffer",     16 * sizeof(GpuMesh),     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    sceneHandles.material = { builder.createPerFrameBuffer("SceneMaterialBuffer", 16 * sizeof(GpuMaterial), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    sceneHandles.object   = { builder.createPerFrameBuffer("SceneObjectBuffer",   sizeof(GpuObjectHeader) + 16 * sizeof(ObjectHandle), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    sceneHandles.light    = { builder.createPerFrameBuffer("SceneLightBuffer",    sizeof(GpuLightHeader)  + 16 * sizeof(GpuLight),      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };

    // Pathtracing pass
    ComputePassBuilder pathtrace = builder.addComputePass("PathtracingPass");
    pathtracePassHandle = pathtrace.getHandle();
    pathtrace.setGroup(mainGroupHandle);
    pathtrace.readBuffer( 0, pathtracingUBOHandle,           BufferUsageType::Uniform);
    pathtrace.readImage ( 1, previousPathtracingImageHandle, ImageUsageType::Sampled);
    pathtrace.readBuffer( 2, pixelInfoBufferHandle,          BufferUsageType::Storage);
    pathtrace.readBuffer( 3, sceneHandles.sphere.handle,     BufferUsageType::Storage);
    pathtrace.readBuffer( 4, sceneHandles.plane.handle,      BufferUsageType::Storage);
    pathtrace.readBuffer( 5, sceneHandles.box.handle,        BufferUsageType::Storage);
    pathtrace.readBuffer( 6, sceneHandles.vertex.handle,     BufferUsageType::Storage);
    pathtrace.readBuffer( 7, sceneHandles.index.handle,      BufferUsageType::Storage);
    pathtrace.readBuffer( 8, sceneHandles.bvh.handle,        BufferUsageType::Storage);
    pathtrace.readBuffer( 9, sceneHandles.mesh.handle,       BufferUsageType::Storage);
    pathtrace.readBuffer(10, sceneHandles.material.handle,   BufferUsageType::Storage);
    pathtrace.readBuffer(11, sceneHandles.object.handle,     BufferUsageType::Storage);
    pathtrace.readBuffer(12, sceneHandles.light.handle,      BufferUsageType::Storage);
    pathtrace.writeImage(13, currentPathtracingImageHandle,  ImageUsageType::Storage);
    pathtracingPipelineHandle = pathtrace.setPipeline("./shaders/pathtracing/pathtracing.glsl");

    // Compositing pass
    ComputePassBuilder composite = builder.addComputePass("CompositionPass");
    compositePassHandle = composite.getHandle();
    composite.setGroup(mainGroupHandle);
    composite.readImage (0, currentPathtracingImageHandle, ImageUsageType::Sampled);
    composite.readBuffer(1, displayUBOHandle,              BufferUsageType::Uniform);
    composite.readBuffer(2, pixelInfoBufferHandle,         BufferUsageType::Storage);
    composite.writeImage(3, outputImageHandle,             ImageUsageType::Storage);
    compositingPipelineHandle = composite.setPipeline("./shaders/compositing.glsl");

    TransferPassBuilder exportPass = builder.addTransferPass("ExportPass");
    exportPassHandle = exportPass.getHandle();
    exportPass.setGroup(mainGroupHandle);
    exportPass.copyFrom(outputImageHandle);

    if (!engine.isHeadless()) {
        GraphicsPassBuilder display = builder.addGraphicsPass("DisplayPass");
        displayPassHandle = display.getHandle();
        display.setGroup(mainGroupHandle);
        display.readImage (0, outputImageHandle,     ImageUsageType::Sampled);
        display.readBuffer(1, displayUBOHandle,      BufferUsageType::Uniform);
        display.readBuffer(2, pixelInfoBufferHandle, BufferUsageType::Storage);
        display.writeImage(swapchainImageHandle, ImageUsageType::ColorAttachment, WriteMode::Overwrite, AttachmentLoad::Clear);
        display.bindVertexBuffer(vertexBufferHandle);
        display.bindIndexBuffer(indexBufferHandle);
        VertexInput<ScreenVertex> vertexInput;
        vertexInput.addAttributeDescription(VK_FORMAT_R32G32_SFLOAT, offsetof(ScreenVertex, pos));
        displayPipelineHandle = display.setPipeline(
            vertexInput.get(),
            {
                { VK_SHADER_STAGE_VERTEX_BIT,   "./shaders/vert.glsl" },
                { VK_SHADER_STAGE_FRAGMENT_BIT, "./shaders/frag.glsl" }
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
    }

    engine.setGraph(builder);
    engine.initGraph();
    ctx.scene->setGpuBufferHandles(sceneHandles);

    if (engine.isHeadless()) {
        VkExtent2D ext = engine.getExtent();
        readbackBuffer = engine.createReadbackBuffer(static_cast<size_t>(ext.width) * ext.height * 4 * sizeof(float));
    } else {
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
        exportService.init(engine, engine.getExtent().width, engine.getExtent().height);
    }
}

void RenderHandler::destroy(AppContext& ctx) {
    VkSmol& engine = *ctx.engine;
    if (engine.isHeadless())
        engine.destroyBuffer(readbackBuffer);
    else
        exportService.destroy(engine);
    engine.destroyGraph();
}

void RenderHandler::buildPipelines(AppContext& ctx) {
    VkSmol& engine = *ctx.engine;
    NotificationHandler& notifications = *ctx.notifications;

    engine.waitIdle();

    try {
        engine.reloadPipelines();
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << std::endl;
        notifications.pushMessage(NotificationType::Error, e.what());
        return;
    }

    std::cout << "[INFO] Built pipelines" << std::endl;
    notifications.pushMessage(NotificationType::Info, "(Re)Built the pipelines");
}

void RenderHandler::render(AppContext& ctx) {
    VkSmol& engine = *ctx.engine;

    uint64_t renderSamplesPerPixel = ctx.parameters->getInt("renderSamples");
    if (ctx.renderState->renderMode != RenderMode::Preview && renderSamplesPerPixel > 0 && !ctx.renderState->pendingExit && !(*ctx.restartRender)) {
        if (ctx.renderState->sampleCount >= renderSamplesPerPixel) {
            exportService.requestSave();
            ctx.renderState->pendingExit = ctx.renderState->renderMode == RenderMode::RenderSingle;
        }
    }

    auto frameContext = engine.beginFrame();
    if (!frameContext) { return; }
    if (frameContext->swapchainGeneration != lastSwapchainGeneration) {
        handleResize(ctx, frameContext->extent);
        lastSwapchainGeneration = frameContext->swapchainGeneration;
        *ctx.restartRender = true;
    }

    ctx.scene->runOnRender(ctx, *frameContext);
    engine.swapBindings(currentPathtracingImageHandle, previousPathtracingImageHandle);

    engine.fillBuffer(engine.getBuffer(pathtracingUBOHandle, frameContext->currentFrame), ctx.pathtracerUBO);
    engine.fillBuffer(engine.getBuffer(displayUBOHandle,     frameContext->currentFrame), ctx.screenUBO);

    engine.bindImage(
        swapchainImageHandle,
        engine.getSwapchainImage(frameContext->imageIndex).get(),
        engine.getSwapchainImageView(frameContext->imageIndex).get());

    pathtracingPass(ctx, *frameContext);
    uiPass(ctx);

    engine.present();
    engine.advanceFrame();

    exportService.handleSave(ctx);
}

void RenderHandler::handleResize(AppContext& ctx, const VkExtent2D& extent) {
    VkSmol& engine = *ctx.engine;
    engine.waitIdle();

    exportService.destroy(engine);
    engine.resizeImage(previousPathtracingImageHandle, extent.width, extent.height);
    engine.resizeImage(currentPathtracingImageHandle,  extent.width, extent.height);
    engine.resizeImage(outputImageHandle,              extent.width, extent.height);
    engine.resizeBuffer(pixelInfoBufferHandle, static_cast<size_t>(extent.width) * extent.height * sizeof(PixelInfo));
    exportService.init(engine, extent.width, extent.height);
}

void RenderHandler::pathtracingPass(AppContext& ctx, const FrameContext& frameContext, bool captureOutput) {
    VkSmol& engine = *ctx.engine;

    CommandBuffer& commandBuffer = engine.beginRecording(mainGroupHandle);

    VkExtent2D extent = frameContext.extent;

    {   // Pathtrace (compute)
        ComputePipeline& ptPipeline = engine.getComputePipeline(pathtracingPipelineHandle);
        engine.emitBarriers(commandBuffer, pathtracePassHandle);
        engine.bindDescriptors(commandBuffer, pathtracePassHandle);
        ptPipeline.bind(commandBuffer);
        ptPipeline.dispatch(commandBuffer, (extent.width + 7) / 8, (extent.height + 7) / 8);
    }

    {   // Compositing (compute)
        ComputePipeline& coPipeline = engine.getComputePipeline(compositingPipelineHandle);
        engine.emitBarriers(commandBuffer, compositePassHandle);
        engine.bindDescriptors(commandBuffer, compositePassHandle);
        coPipeline.bind(commandBuffer);
        coPipeline.dispatch(commandBuffer, (extent.width + 7) / 8, (extent.height + 7) / 8);
    }

    engine.emitBarriers(commandBuffer, exportPassHandle);
    if (engine.isHeadless()) {
        if (captureOutput)
            engine.getImage(outputImageHandle).copyToBuffer(commandBuffer, readbackBuffer);
    } else {
        exportService.handleCopy(ctx, commandBuffer, engine.getImage(outputImageHandle));

        {   // Display
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
        }
    }

    engine.endRecording(mainGroupHandle);
}

void RenderHandler::renderHeadless(AppContext& ctx, bool captureOutput) {
    VkSmol& engine = *ctx.engine;

    auto frameContext = engine.beginFrame();
    if (!frameContext) return;

    ctx.scene->runPreUpdate(ctx);
    ctx.scene->runOnRender(ctx, *frameContext);
    engine.swapBindings(currentPathtracingImageHandle, previousPathtracingImageHandle);

    engine.fillBuffer(engine.getBuffer(pathtracingUBOHandle, frameContext->currentFrame), ctx.pathtracerUBO);
    engine.fillBuffer(engine.getBuffer(displayUBOHandle,     frameContext->currentFrame), ctx.screenUBO);

    pathtracingPass(ctx, *frameContext, captureOutput);

    engine.advanceFrame();
}

void RenderHandler::saveCapture(AppContext& ctx, const std::string& path, uint32_t width, uint32_t height) {
    VkSmol& engine = *ctx.engine;
    engine.waitIdle();

    size_t floatCount = static_cast<size_t>(width) * height * 4;
    std::vector<float> floatPixels(floatCount);
    engine.readBuffer(readbackBuffer, floatPixels.data(), floatCount * sizeof(float));

    std::vector<uint8_t> pixels(floatCount);
    auto toByte = [](float v) -> uint8_t {
        const float a = 2.51f, b = 0.03f, c = 2.43f, d = 0.59f, e = 0.14f;
        v = std::clamp((v * (a * v + b)) / (v * (c * v + d) + e), 0.0f, 1.0f);
        v = std::pow(v, 1.0f / 2.2f);
        return static_cast<uint8_t>(v * 255.0f + 0.5f);
    };
    for (size_t i = 0; i < floatCount; i += 4) {
        pixels[i + 0] = toByte(floatPixels[i + 0]);
        pixels[i + 1] = toByte(floatPixels[i + 1]);
        pixels[i + 2] = toByte(floatPixels[i + 2]);
        pixels[i + 3] = 255;
    }

    if (stbi_write_png(path.c_str(), static_cast<int>(width), static_cast<int>(height), 4, pixels.data(), static_cast<int>(width) * 4))
        std::cout << "[RenderHandler] Saved capture to " << path << std::endl;
    else
        std::cerr << "[RenderHandler] Failed to write " << path << std::endl;
}

void RenderHandler::uiPass(AppContext& ctx) {
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
