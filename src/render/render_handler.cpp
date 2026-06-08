#include "render_handler.hpp"

#include <iostream>

#include "./imgui/imgui.h"

#include "engine/engine.hpp"
#include "engine/graph/graph_utils.hpp"
#include "engine/graph/graphics_pass_builder.hpp"
#include "engine/graph/present_pass_builder.hpp"
#include "engine/graph/transfer_pass_builder.hpp"
#include "engine/graph/render_graph_builder.hpp"
#include "engine/graph/builder_resource.hpp"

#include "app/app_context.hpp"
#include "app/notification_handler.hpp"
#include "app/parameter_handler.hpp"
#include "engine/graph/render_graph_executor.hpp"
#include "engine/image/image.hpp"
#include "scene/scene.hpp"
#include "editor/editor_ui.hpp"

void RenderHandler::init(AppContext& ctx) {
    VkSmol& engine = *ctx.engine;

    {
        RenderGraphBuilder builder;
        swapchainImageHandle = builder.importImage(
            "SwapchainImage",
            VK_FORMAT_R32G32B32A32_SFLOAT,
            engine.getExtent().width, engine.getExtent().height, 1,
            { .usage = ImageUsageType::Undefined, .access = AccessType::None },
            { .usage = ImageUsageType::Present, .access = AccessType::Read }
        );
        previousPathtracingImageHandle = builder.importImage(
            "PreviousPathtracingImage",
            VK_FORMAT_R32G32B32A32_SFLOAT,
            engine.getExtent().width, engine.getExtent().height, 1,
            { .usage = ImageUsageType::Sampled, .access = AccessType::Read },
            { .usage = ImageUsageType::Sampled, .access = AccessType::Read }
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

        GraphicsPassBuilder pathtrace = builder.addGraphicsPass("PathtracingPass");
        pathtracePassHandle = pathtrace.getHandle();
        pathtrace.readImage(previousPathtracingImageHandle, ImageUsageType::Sampled);
        pathtrace.writeImage(
            currentPathtracingImageHandle,
            ImageUsageType::ColorAttachment,
            WriteMode::Overwrite,
            AttachmentLoad::Clear
        );

        GraphicsPassBuilder composite = builder.addGraphicsPass("CompositionPass");
        compositePassHandle = composite.getHandle();
        composite.readImage(currentPathtracingImageHandle, ImageUsageType::Sampled);
        composite.writeImage(
            outputImageHandle,
            ImageUsageType::ColorAttachment,
            WriteMode::Overwrite,
            AttachmentLoad::DontCare
        );

        TransferPassBuilder exportPass = builder.addTransferPass("ExportPass");
        exportPassHandle = exportPass.getHandle();
        exportPass.copyFrom(outputImageHandle);

        GraphicsPassBuilder display = builder.addGraphicsPass("DisplayPass");
        displayPassHandle = display.getHandle();
        display.readImage(outputImageHandle, ImageUsageType::Sampled);
        display.writeImage(
            swapchainImageHandle,
            ImageUsageType::ColorAttachment,
            WriteMode::Overwrite,
            AttachmentLoad::Clear
        );

        GraphicsPassBuilder ui = builder.addGraphicsPass("UiPass");
        uiPassHandle = ui.getHandle();
        ui.writeImage(
            swapchainImageHandle,
            ImageUsageType::ColorAttachment,
            WriteMode::Preserve,
            AttachmentLoad::Load
        );

        PresentPassBuilder present = builder.addPresentPass("PresentPass");
        presentPassHandle = present.getHandle();
        present.setPresentationImage(swapchainImageHandle);

        executor.setCompiledGraph(builder.compile());
    }

    // =============================================================

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;

    {   // Buffer creation
        vertexBuffer = engine.createBuffer(
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            sizeof(ScreenVertex) * vertices.size(), (void*)vertices.data(),
            "FullscreenVertexBuffer"
        );
        
        indexBuffer = engine.createBuffer(
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            sizeof(index_t) * indices.size(), (void*)indices.data(),
            "FullscreenIndexBuffer"
        );
    
        pathtracingUniformBuffers = engine.createPerFrameBuffer<PathtracerUBO>(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 1, "PathtracingUniformBuffer");
        displayUniformBuffers = engine.createPerFrameBuffer<ScreenUBO>(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 1, "DisplayUniformBuffer");

        VkExtent2D extent = engine.getExtent();
        size_t pixelInfoCount = static_cast<size_t>(extent.width) * extent.height;
        pixelInfoBuffer = engine.createSharedBuffer<PixelInfo>(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, pixelInfoCount, "PixelInfoBuffer");
    }

    {   // Image (image + view + combinedImageSampler) creation
        VkExtent2D extent = engine.getExtent();
        for (size_t i = 0; i < 2; i++) {
            pathtracingImages[i] = engine.createImage(
                extent.width, extent.height,
                VK_FORMAT_R32G32B32A32_SFLOAT,  // Use 32 bit float format to avoid quantizing every accumulation step
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                "PathtracingImage[" + std::to_string(i) + "]"
            );
            pathtracingImageViews[i] = engine.createImageView(pathtracingImages[i], "PathtracingImageView[" + std::to_string(i) + "]");
            pathtracingSamplers[i] = engine.createSampler("PathtracingSampler[" + std::to_string(i) + "]");
        }

        
        outputImage = engine.createImage(
            extent.width, extent.height,
            VK_FORMAT_R32G32B32A32_SFLOAT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            "OutputImage"
        );
        
        outputImageView = engine.createImageView(outputImage, "OutputImageView");
        outputSampler = engine.createSampler("OutputSampler");
        executor.bindImage(outputImageHandle, outputImage.get(),outputImageView.get());
        
        exportService.init(engine, extent.width, extent.height);
    }

    pathtracingSetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    pathtracingSetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    pathtracingSetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    pathtracingSetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    pathtracingSetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    pathtracingSetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    pathtracingSetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    pathtracingSetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    pathtracingSetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    pathtracingSetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    pathtracingSetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    pathtracingSetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    pathtracingSetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    engine.initDescriptorSetLayout(pathtracingSetLayout);
    
    compositingSetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    compositingSetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    compositingSetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    engine.initDescriptorSetLayout(compositingSetLayout);

    displaySetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    displaySetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    displaySetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    engine.initDescriptorSetLayout(displaySetLayout);

    {   // Descriptor sets creation
        SceneGpuBuffers& buffers = ctx.scene->getBuffers();
        pathtracingDescriptorSet = engine.createDescriptorSetGroup(pathtracingSetLayout, "PathtracingDescriptorSet");
        compositingDescriptorSet = engine.createDescriptorSetGroup(compositingSetLayout, "CompositingDescriptorSet");
        displayDescriptorSet = engine.createDescriptorSetGroup(displaySetLayout, "DisplayDescriptorSet");

        for (uint32_t frameIndex = 0; frameIndex < MAX_FRAME_IN_FLIGHT; ++frameIndex) {
            DescriptorWriter pathtracingWriter(pathtracingSetLayout);
            pathtracingWriter.uniform(0, pathtracingUniformBuffers.at(frameIndex))
                .combinedImageSampler(1, pathtracingImageViews[1 - frame], pathtracingSamplers[1 - frame])
                .storage(2, pixelInfoBuffer.get())
                .storage(3, buffers.sphere.at(frameIndex))
                .storage(4, buffers.plane.at(frameIndex))
                .storage(5, buffers.box.at(frameIndex))
                .storage(6, buffers.vertex.at(frameIndex))
                .storage(7, buffers.index.at(frameIndex))
                .storage(8, buffers.bvh.at(frameIndex))
                .storage(9, buffers.mesh.at(frameIndex))
                .storage(10, buffers.material.at(frameIndex))
                .storage(11, buffers.object.at(frameIndex))
                .storage(12, buffers.light.at(frameIndex));
            
            pathtracingWriter.validate();
            engine.updateDescriptorSetGroup(pathtracingDescriptorSet, frameIndex, pathtracingWriter);
            
            DescriptorWriter compositingWriter(compositingSetLayout);
            compositingWriter.combinedImageSampler(0, pathtracingImageViews[frame], pathtracingSamplers[frame])
                .uniform(1, displayUniformBuffers.at(frameIndex))
                .storage(2, pixelInfoBuffer.get());
            engine.updateDescriptorSetGroup(compositingDescriptorSet, frameIndex, compositingWriter);
            
            DescriptorWriter displayWriter(displaySetLayout);
            displayWriter.combinedImageSampler(0, outputImageView, outputSampler)
                .uniform(1, displayUniformBuffers.at(frameIndex))
                .storage(2, pixelInfoBuffer.get());
            engine.updateDescriptorSetGroup(displayDescriptorSet, frameIndex, displayWriter);
        }
    }

    buildPipelines(ctx);
}

void RenderHandler::destroy(AppContext& ctx) {
    VkSmol& engine = *ctx.engine;

    engine.destroyDescriptorSetLayout(pathtracingSetLayout);
    engine.destroyDescriptorSetLayout(compositingSetLayout);
    engine.destroyDescriptorSetLayout(displaySetLayout);

    destroyDescriptors(ctx);
    destroyImages(ctx);

    engine.destroyPerFrameBuffer(pathtracingUniformBuffers);
    engine.destroyPerFrameBuffer(displayUniformBuffers);

    engine.destroyBuffer(vertexBuffer);
    engine.destroyBuffer(indexBuffer);
    
    engine.destroyGraphicsPipeline(pathtracingPipeline);
    engine.destroyGraphicsPipeline(compositingPipeline);
    engine.destroyGraphicsPipeline(displayPipeline);
}

void RenderHandler::buildPipelines(AppContext& ctx) {
    VkSmol& engine = *ctx.engine;
    NotificationHandler& notifications = *ctx.notifications;

    engine.waitIdle();

    std::string vertShaderPath = "./shaders/vert.glsl";
    std::string pathtracingFragShaderPath = "./shaders/pathtracing/pathtracing.glsl";
    std::string compositingFragShaderPath = "./shaders/compositing.glsl";
    std::string displayFragShaderPath = "./shaders/frag.glsl";
    
    Shader vertShader;
    Shader pathtracingFragShader, compositingFragShader, displayFragShader;
    
    // Compile the vertex shader
    try {
        vertShader = engine.createShader(VK_SHADER_STAGE_VERTEX_BIT, vertShaderPath);
    } catch (...) {
        std::cerr << "[ERROR] Failed to compile shader [" << vertShaderPath << "]: pipeline not built" << std::endl;
        notifications.pushMessage(
            NotificationType::Error,
            "Failed to compile shader [" + vertShaderPath + "]: pipeline not built"
        );
        return;
    }

    VertexInput<ScreenVertex> vertexInput;
    vertexInput.addAttributeDescription(VK_FORMAT_R32G32_SFLOAT, offsetof(ScreenVertex, pos));
    
    // Build the pathtracing pipeline
    {
        try {
            pathtracingFragShader = engine.createShader(VK_SHADER_STAGE_FRAGMENT_BIT, pathtracingFragShaderPath);
        } catch (...) {
            engine.destroyShader(vertShader);
            std::cerr << "[ERROR] Failed to compile shader [" << pathtracingFragShaderPath << "]: pipeline not built" << std::endl;
            notifications.pushMessage(
                NotificationType::Error,
                "Failed to compile shader [" + pathtracingFragShaderPath + "]: pipeline not built"
            );
            return;
        }
        
        if (pathtracingPipeline.get() != VK_NULL_HANDLE) {
            GraphicsPipeline newPipeline = engine.initGraphicsPipeline(
                vertexInput.get(),
                { &vertShader, &pathtracingFragShader },
                { &pathtracingSetLayout },
                pathtracingPipeline,
                VK_FORMAT_R32G32B32A32_SFLOAT
            );
            engine.destroyGraphicsPipeline(pathtracingPipeline);
            pathtracingPipeline = newPipeline;
        } else {
            pathtracingPipeline = engine.initGraphicsPipeline(
                vertexInput.get(),
                { &vertShader, &pathtracingFragShader },
                { &pathtracingSetLayout },
                GraphicsPipeline(),
                VK_FORMAT_R32G32B32A32_SFLOAT
            );
        }
    }

    // Build the compositing pipeline
    {
        try {
            compositingFragShader = engine.createShader(VK_SHADER_STAGE_FRAGMENT_BIT, compositingFragShaderPath);
        } catch (...) {
            engine.destroyShader(vertShader);
            engine.destroyShader(pathtracingFragShader);
            std::cerr << "[ERROR] Failed to compile shader [" << compositingFragShaderPath << "]: compositing pipeline not built" << std::endl;
            notifications.pushMessage(
                NotificationType::Error,
                "Failed to compile shader [" + compositingFragShaderPath + "]: compositing pipeline not built"
            );
            return;
        }
    
        if (compositingPipeline.get() != VK_NULL_HANDLE) {
            GraphicsPipeline newCompositingPipeline = engine.initGraphicsPipeline(
                vertexInput.get(),
                { &vertShader, &compositingFragShader },
                { &compositingSetLayout },
                compositingPipeline,
                VK_FORMAT_R32G32B32A32_SFLOAT
            );
            engine.destroyGraphicsPipeline(compositingPipeline);
            compositingPipeline = newCompositingPipeline;
        } else {
            compositingPipeline = engine.initGraphicsPipeline(
                vertexInput.get(),
                { &vertShader, &compositingFragShader },
                { &compositingSetLayout },
                GraphicsPipeline(),
                VK_FORMAT_R32G32B32A32_SFLOAT
            );
        }
    }

    // Build the display pipeline
    {
        try {
            displayFragShader = engine.createShader(VK_SHADER_STAGE_FRAGMENT_BIT, displayFragShaderPath);
        } catch (...) {
            engine.destroyShader(vertShader);
            engine.destroyShader(pathtracingFragShader);
            engine.destroyShader(compositingFragShader);
            std::cerr << "[ERROR] Failed to compile shader [" << displayFragShaderPath << "]: display pipeline not built" << std::endl;
            notifications.pushMessage(
                NotificationType::Error,
                "Failed to compile shader [" + displayFragShaderPath + "]: display pipeline not built"
            );
            return;
        }
    
        if (displayPipeline.get() != VK_NULL_HANDLE) {
            GraphicsPipeline newDisplayPipeline = engine.initGraphicsPipeline(
                vertexInput.get(),
                { &vertShader, &displayFragShader },
                { &displaySetLayout },
                displayPipeline
            );
            engine.destroyGraphicsPipeline(displayPipeline);
            displayPipeline = newDisplayPipeline;
        } else {
            displayPipeline = engine.initGraphicsPipeline(
                vertexInput.get(),
                { &vertShader, &displayFragShader },
                { &displaySetLayout }
            );
        }
    }

    engine.destroyShader(vertShader);
    engine.destroyShader(pathtracingFragShader);
    engine.destroyShader(compositingFragShader);
    engine.destroyShader(displayFragShader);

    std::cout << "[INFO] Built pipelines by recompiling [" << vertShaderPath << "], [" << pathtracingFragShaderPath << "], [" << compositingFragShaderPath << "], and [" << displayFragShaderPath << "]" << std::endl;
    notifications.pushMessage(
        NotificationType::Info,
        "(Re)Built the pipelines"
    );
}

void RenderHandler::render(AppContext& ctx) {
    VkSmol& engine = *ctx.engine;

    uint64_t renderSamplesPerPixel = ctx.parameters->getInt("renderSamples");
    if (ctx.renderState->renderMode != RenderMode::Preview && renderSamplesPerPixel > 0 && !ctx.renderState->pendingExit && !(*ctx.restartRender)) {
        if (ctx.renderState->sampleCount >= renderSamplesPerPixel) {
            exportService.requestSave();
            ctx.renderState->pendingExit = ctx.renderState->renderMode == RenderMode::RenderSingle; // we don't want to exist when rendering an animation
        }
    }
    
    auto frameContext = engine.beginFrame();
    if (!frameContext) {
        return;
    }
    if (frameContext->swapchainGeneration != lastSwapchainGeneration) {
        handleResize(ctx, frameContext->extent);
        lastSwapchainGeneration = frameContext->swapchainGeneration;
        *ctx.restartRender = true;
        frame = 0;
    }
    
    ctx.scene->runOnRender(ctx, *frameContext);
    frame = (frame + 1) % 2;

    // Rebuild descriptor set
    if (ctx.scene->checkBufferUpdate()) {
        SceneGpuBuffers& buffers = ctx.scene->getBuffers();
        DescriptorSetGroup newGroup = engine.createDescriptorSetGroup(pathtracingSetLayout, "PathtracingDescriptorSet");
        
        if (newGroup.isValide()) {
            engine.destroyDescriptorSetGroup(pathtracingDescriptorSet);
            pathtracingDescriptorSet = std::move(newGroup);

            for (uint32_t frameIndex = 0; frameIndex < MAX_FRAME_IN_FLIGHT; ++frameIndex) {
                DescriptorWriter pathtracingWriter(pathtracingSetLayout);
                pathtracingWriter.uniform(0, pathtracingUniformBuffers.at(frameIndex))
                    .combinedImageSampler(1, pathtracingImageViews[1 - frame], pathtracingSamplers[1 - frame])
                    .storage(2, pixelInfoBuffer.get())
                    .storage(3, buffers.sphere.at(frameIndex))
                    .storage(4, buffers.plane.at(frameIndex))
                    .storage(5, buffers.box.at(frameIndex))
                    .storage(6, buffers.vertex.at(frameIndex))
                    .storage(7, buffers.index.at(frameIndex))
                    .storage(8, buffers.bvh.at(frameIndex))
                    .storage(9, buffers.mesh.at(frameIndex))
                    .storage(10, buffers.material.at(frameIndex))
                    .storage(11, buffers.object.at(frameIndex))
                    .storage(12, buffers.light.at(frameIndex));
                engine.updateDescriptorSetGroup(pathtracingDescriptorSet, frameIndex, pathtracingWriter);

                DescriptorWriter compositingWriter(compositingSetLayout);
            compositingWriter.combinedImageSampler(0, pathtracingImageViews[frame], pathtracingSamplers[frame])
                .uniform(1, displayUniformBuffers.at(frameIndex))
                .storage(2, pixelInfoBuffer.get());
            engine.updateDescriptorSetGroup(compositingDescriptorSet, frameIndex, compositingWriter);
            }
        }
    } else {
        DescriptorWriter pathtracingWriter(pathtracingSetLayout);
        pathtracingWriter.uniform(0, pathtracingUniformBuffers.at(frameContext->currentFrame))
            .combinedImageSampler(1, pathtracingImageViews[1 - frame], pathtracingSamplers[1 - frame]);
        engine.updateDescriptorSetGroup(pathtracingDescriptorSet, frameContext->currentFrame, pathtracingWriter);

        DescriptorWriter compositingWriter(compositingSetLayout);
        compositingWriter.combinedImageSampler(0, pathtracingImageViews[frame], pathtracingSamplers[frame])
            .uniform(1, displayUniformBuffers.at(frameContext->currentFrame));
        engine.updateDescriptorSetGroup(compositingDescriptorSet, frameContext->currentFrame, compositingWriter);
    }
    
    
    engine.fillBuffer(pathtracingUniformBuffers.current(frameContext.value()), ctx.pathtracerUBO);
    engine.fillBuffer(displayUniformBuffers.current(frameContext.value()), ctx.screenUBO);

    executor.bindImage(
        previousPathtracingImageHandle,
        pathtracingImages[1-frame].get(),
        pathtracingImageViews[1-frame].get());
    executor.bindImage(
        currentPathtracingImageHandle, 
        pathtracingImages[frame].get(), 
        pathtracingImageViews[frame].get());
    executor.bindImage(
        swapchainImageHandle,
        engine.getSwapchainImage(frameContext->imageIndex),
        engine.getSwapchainImageView(frameContext->imageIndex));
    
    pathtracingPass(ctx, *frameContext);
    uiPass(ctx);

    engine.endFrame();

    exportService.handleSave(ctx);
}

void RenderHandler::destroyDescriptors(AppContext& ctx) {
    VkSmol& engine = *ctx.engine;

    engine.destroyDescriptorSetGroup(pathtracingDescriptorSet);
    engine.destroyDescriptorSetGroup(compositingDescriptorSet);
    engine.destroyDescriptorSetGroup(displayDescriptorSet);
}

void RenderHandler::destroyImages(AppContext& ctx) {
    VkSmol& engine = *ctx.engine;

    for (size_t i = 0; i < 2; i++) {
        engine.destroySampler(pathtracingSamplers[i]);
        engine.destroyImageView(pathtracingImageViews[i]);
        engine.destroyImage(pathtracingImages[i]);
    }
    engine.destroySampler(outputSampler);
    engine.destroyImageView(outputImageView);
    engine.destroyImage(outputImage);

    engine.destroySharedBuffer(pixelInfoBuffer);
    exportService.destroy(engine);
}

void RenderHandler::rebuildImages(AppContext& ctx, const VkExtent2D& extent) {
    VkSmol& engine = *ctx.engine;

    for (size_t i = 0; i < 2; i++) {
        pathtracingImages[i] = engine.createImage(
            extent.width, extent.height,
            VK_FORMAT_R32G32B32A32_SFLOAT,  // Use 32 bit float format to avoid quantizing every accumulation step
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            "PathtracingImage[" + std::to_string(i) + "]"
        );
        pathtracingImageViews[i] = engine.createImageView(pathtracingImages[i], "PathtracingImageView[" + std::to_string(i) + "]");
        pathtracingSamplers[i] = engine.createSampler("PathtracingSampler[" + std::to_string(i) + "]");
    }
    outputImage = engine.createImage(
        extent.width, extent.height,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        "OutputImage"
    );
    outputImageView = engine.createImageView(outputImage, "OutputImageView");
    outputSampler = engine.createSampler("OutputSampler");
    executor.bindImage(outputImageHandle, outputImage.get(),outputImageView.get());

    // These are not strictly speaking images, but they hold per pixel information
    size_t pixelInfoCount = static_cast<size_t>(extent.width) * extent.height;
    pixelInfoBuffer = engine.createSharedBuffer<PixelInfo>(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, pixelInfoCount, "PixelInfoBuffer");

    exportService.init(engine, extent.width, extent.height);
}

void RenderHandler::rebuildDescriptors(AppContext& ctx) {
    VkSmol& engine = *ctx.engine;

    SceneGpuBuffers& buffers = ctx.scene->getBuffers();
    pathtracingDescriptorSet = engine.createDescriptorSetGroup(pathtracingSetLayout, "PathtracingDescriptorSet");
    compositingDescriptorSet = engine.createDescriptorSetGroup(compositingSetLayout, "CompositingDescriptorSet");
    displayDescriptorSet = engine.createDescriptorSetGroup(displaySetLayout, "DisplayDescriptorSet");

    for (uint32_t frameIndex = 0; frameIndex < MAX_FRAME_IN_FLIGHT; ++frameIndex) {
        DescriptorWriter pathtracingWriter(pathtracingSetLayout);
        pathtracingWriter.uniform(0, pathtracingUniformBuffers.at(frameIndex))
            .combinedImageSampler(1, pathtracingImageViews[1 - frame], pathtracingSamplers[1 - frame])
            .storage(2, pixelInfoBuffer.get())
            .storage(3, buffers.sphere.at(frameIndex))
            .storage(4, buffers.plane.at(frameIndex))
            .storage(5, buffers.box.at(frameIndex))
            .storage(6, buffers.vertex.at(frameIndex))
            .storage(7, buffers.index.at(frameIndex))
            .storage(8, buffers.bvh.at(frameIndex))
            .storage(9, buffers.mesh.at(frameIndex))
            .storage(10, buffers.material.at(frameIndex))
            .storage(11, buffers.object.at(frameIndex))
            .storage(12, buffers.light.at(frameIndex));
        // uint32_t binding = 3;
        // for (PerFrameBufferBase* buffers : storageBuffers) {
        // // for (bufferList_t& buffers : storageBuffers) {
        //     pathtracingWriter.storage(binding, buffers->at(frameIndex));
        //     // pathtracingWriter.storage(binding, engine.getBuffer(buffers, frameIndex));
        //     binding++;
        // }
        engine.updateDescriptorSetGroup(pathtracingDescriptorSet, frameIndex, pathtracingWriter);
        
        DescriptorWriter compositingWriter(compositingSetLayout);
        compositingWriter.combinedImageSampler(0, pathtracingImageViews[frame], pathtracingSamplers[frame])
            .uniform(1, displayUniformBuffers.at(frameIndex))
            .storage(2, pixelInfoBuffer.get());
        engine.updateDescriptorSetGroup(compositingDescriptorSet, frameIndex, compositingWriter);
        
        DescriptorWriter displayWriter(displaySetLayout);
        displayWriter.combinedImageSampler(0, outputImageView, outputSampler)
            .uniform(1, displayUniformBuffers.at(frameIndex))
            .storage(2, pixelInfoBuffer.get());
        engine.updateDescriptorSetGroup(displayDescriptorSet, frameIndex, displayWriter);
    }
}

void RenderHandler::handleResize(AppContext& ctx, const VkExtent2D& extent) {
    VkSmol& engine = *ctx.engine;

    engine.waitIdle();

    destroyImages(ctx);
    destroyDescriptors(ctx);

    rebuildImages(ctx, extent);
    rebuildDescriptors(ctx);
}

void RenderHandler::pathtracingPass(AppContext& ctx, const FrameContext& frameContext) {
    VkSmol& engine = *ctx.engine;

    CommandBuffer& commandBuffer = engine.beginRecordingRender();

    VkExtent2D extent = frameContext.extent;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)extent.width;
    viewport.height = (float)extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    VkRect2D scissor;
    scissor.offset = {0, 0};
    scissor.extent = extent;
    
    {   // Pathtrace
        executor.emitBarriers(commandBuffer, pathtracePassHandle);
        std::vector<AttachmentInfo> attachments = executor.getColorAttachment(pathtracePassHandle);
        assert(attachments.size() == 1);
        engine.beginDynamicRenderer(
            commandBuffer,
            attachments[0].view, attachments[0].layout,
            attachments[0].loadOp, VK_ATTACHMENT_STORE_OP_STORE,
            {{ 0.0f, 0.0f, 0.0f, 1.0f }}
        );
        
        // Bind current descriptor set
        pathtracingDescriptorSet.at(frameContext.currentFrame).bind(commandBuffer, pathtracingPipeline.getLayout());
        
        pathtracingPipeline.bind(commandBuffer);

        vertexBuffer.bindVertex(commandBuffer);
        indexBuffer.bindIndex(commandBuffer, VK_INDEX_TYPE_UINT16);
        
        pathtracingPipeline.setViewport(commandBuffer, viewport);
        pathtracingPipeline.setScissor(commandBuffer, scissor);
        pathtracingPipeline.drawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()));

        engine.endDynamicRenderer(commandBuffer);

    }

    {   // Compositing
        executor.emitBarriers(commandBuffer, compositePassHandle);
        std::vector<AttachmentInfo> attachments = executor.getColorAttachment(compositePassHandle);
        assert(attachments.size() == 1);
        engine.beginDynamicRenderer(
            commandBuffer,
            attachments[0].view, attachments[0].layout,
            attachments[0].loadOp, VK_ATTACHMENT_STORE_OP_STORE,
            {{ 0.0f, 0.0f, 0.0f, 1.0f }}
        );

        compositingDescriptorSet.at(frameContext.currentFrame).bind(commandBuffer, compositingPipeline.getLayout());
        compositingPipeline.bind(commandBuffer);

        vertexBuffer.bindVertex(commandBuffer);
        indexBuffer.bindIndex(commandBuffer, VK_INDEX_TYPE_UINT16);

        compositingPipeline.setViewport(commandBuffer, viewport);
        compositingPipeline.setScissor(commandBuffer, scissor);
        compositingPipeline.drawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()));

        engine.endDynamicRenderer(commandBuffer);
    }

    executor.emitBarriers(commandBuffer, exportPassHandle);
    exportService.handleCopy(ctx, commandBuffer, outputImage);

    {   // Display
        executor.emitBarriers(commandBuffer, displayPassHandle);
        std::vector<AttachmentInfo> attachments = executor.getColorAttachment(displayPassHandle);
        assert(attachments.size() == 1);
        engine.beginDynamicRenderer(
            commandBuffer,
            attachments[0].view, attachments[0].layout,
            attachments[0].loadOp, VK_ATTACHMENT_STORE_OP_STORE,
            {{ 0.0f, 0.0f, 0.0f, 1.0f }}
        );
        
        displayDescriptorSet.at(frameContext.currentFrame).bind(commandBuffer, displayPipeline.getLayout());

        displayPipeline.bind(commandBuffer);

        vertexBuffer.bindVertex(commandBuffer);
        indexBuffer.bindIndex(commandBuffer, VK_INDEX_TYPE_UINT16);
        
        displayPipeline.setViewport(commandBuffer, viewport);
        displayPipeline.setScissor(commandBuffer, scissor);
        displayPipeline.drawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()));

        engine.endDynamicRenderer(commandBuffer);
    }

    engine.endRecoringRender(commandBuffer);
}

void RenderHandler::uiPass(AppContext& ctx) {
    VkSmol& engine = *ctx.engine;

    CommandBuffer& commandBuffer = engine.beginRecordingUiRender();

    executor.emitBarriers(commandBuffer, uiPassHandle);
    std::vector<AttachmentInfo> attachments = executor.getColorAttachment(uiPassHandle);
    assert(attachments.size() == 1);
    engine.beginDynamicRenderer(
        commandBuffer,
        attachments[0].view, attachments[0].layout,
        attachments[0].loadOp, VK_ATTACHMENT_STORE_OP_STORE,
        {{ 0.0f, 0.0f, 0.0f, 1.0f }}
    );
    
    ctx.ui->draw(commandBuffer, ctx);
    
    engine.endDynamicRenderer(commandBuffer);
    
    executor.emitBarriers(commandBuffer, presentPassHandle);

    // @warning this is out of place, but I need a command buffer
    executor.emitOutputBarriers(commandBuffer);

    engine.endRecoringUiRender(commandBuffer);
}
