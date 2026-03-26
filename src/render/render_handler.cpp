#include "render_handler.hpp"

#include <iostream>

#include "./imgui/imgui.h"

#include "engine/descriptor/descriptor_set_allocation.hpp"
#include "engine/engine.hpp"
#include "engine/pipeline/vertex_input.hpp"

#include "app/notification_handler.hpp"
#include "app/parameter_handler.hpp"
#include "scene/scene.hpp"
#include "editor/editor_ui.hpp"

void RenderHandler::init(AppContext& ctx) {
    VkSmol& engine = *ctx.engine;

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;

    {   // Buffer creation
        vertexBuffer = engine.initBuffer(
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            sizeof(ScreenVertex) * vertices.size(), (void*)vertices.data()
        );
        
        indexBuffer = engine.initBuffer(
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            sizeof(index_t) * indices.size(), (void*)indices.data()
        );
    
        pathtracingUniformBuffers = engine.initBufferList(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(PathtracerUBO));
        displayUniformBuffers = engine.initBufferList(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(ScreenUBO));

        VkExtent2D extent = engine.getExtent();
        size_t pixelInfoBytes = static_cast<size_t>(extent.width) * extent.height * sizeof(PixelInfo);
        pixelInfoBuffers = engine.initSharedBufferList(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, pixelInfoBytes);
    }

    {   // Image (image + view + sampler) creation
        VkExtent2D extent = engine.getExtent();
        for (size_t i = 0; i < 2; i++) {
            pathtracingImages[i] = engine.initImage(
                extent.width, extent.height,
                VK_FORMAT_R32G32B32A32_SFLOAT,  // Use 32 bit float format to avoid quantizing every accumulation step
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            );
            pathtracingImageViews[i] = engine.initImageView(pathtracingImages[i]);
            pathtracingSamplers[i] = engine.initSampler();
        }
        outputImage = engine.initImage(
            extent.width, extent.height,
            VK_FORMAT_R32G32B32A32_SFLOAT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );
        outputImageView = engine.initImageView(outputImage);
        outputSampler = engine.initSampler();

        exportService.init(engine, extent.width, extent.height);
    }

    pathtracingSetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    pathtracingSetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    pathtracingSetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    for (size_t i = 0; i < ctx.scene->getBufferLists().size(); i++) {
        pathtracingSetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    }
    engine.initDescriptorSetLayout(pathtracingSetLayout);
    
    compositingSetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    compositingSetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    compositingSetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    engine.initDescriptorSetLayout(compositingSetLayout);

    displaySetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    displaySetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    displaySetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    engine.initDescriptorSetLayout(displaySetLayout);
    
    buildPipelines(ctx);

    {   // Descriptor sets creation
        std::vector<std::pair<ImageView, Sampler> > pathtracingCombinedImageSampler = {
            { pathtracingImageViews[0], pathtracingSamplers[0] },
            { pathtracingImageViews[1], pathtracingSamplers[1] }
        };
        std::vector<std::pair<ImageView, Sampler> > outputCombinedImageSampler = {
            { outputImageView, outputSampler },
            { outputImageView, outputSampler }
        };
        
        std::vector<bufferList_t> storageBuffers = ctx.scene->getBufferLists();
        for (size_t i = 0; i < 2; i++) {
            std::vector<void*> descriptors = { &pathtracingUniformBuffers, &pathtracingCombinedImageSampler[1-i], &pixelInfoBuffers };
            for (bufferList_t &buffers : storageBuffers) {
                descriptors.push_back(&buffers);
            }

            pathtracingDescriptorSets[i] = engine.createDescriptorSetAllocation(pathtracingSetLayout, descriptors);

            compositingDescriptorSets[i] = engine.createDescriptorSetAllocation(
                compositingSetLayout,
                { &pathtracingCombinedImageSampler[i], &displayUniformBuffers, &pixelInfoBuffers }
            );

            displayDescriptorSets[i] = engine.createDescriptorSetAllocation(
                displaySetLayout,
                { &outputCombinedImageSampler[i], &displayUniformBuffers, &pixelInfoBuffers }
            );
        }
    }
}

void RenderHandler::destroy(AppContext& ctx) {
    VkSmol& engine = *ctx.engine;

    engine.destroyDescriptorSetLayout(pathtracingSetLayout);
    engine.destroyDescriptorSetLayout(compositingSetLayout);
    engine.destroyDescriptorSetLayout(displaySetLayout);

    for (size_t i = 0; i < 2; i++) {
        engine.destroySampler(pathtracingSamplers[i]);
        engine.destroyImage(pathtracingImages[i]);
        engine.destroyImageView(pathtracingImageViews[i]);

        engine.destroyDescriptorSetAllocation(pathtracingDescriptorSets[i]);
        engine.destroyDescriptorSetAllocation(compositingDescriptorSets[i]);
        engine.destroyDescriptorSetAllocation(displayDescriptorSets[i]);
    }
    engine.destroySampler(outputSampler);
    engine.destroyImage(outputImage);
    engine.destroyImageView(outputImageView);

    engine.destroyBuffer(vertexBuffer);
    engine.destroyBuffer(indexBuffer);
    engine.destroyBufferList(pathtracingUniformBuffers);
    engine.destroyBufferList(displayUniformBuffers);
    engine.destroyBufferList(pixelInfoBuffers);
    exportService.destroy(engine);
    
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
        vertShader = engine.initShader(VK_SHADER_STAGE_VERTEX_BIT, vertShaderPath);
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
            pathtracingFragShader = engine.initShader(VK_SHADER_STAGE_FRAGMENT_BIT, pathtracingFragShaderPath);
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
                { vertShader, pathtracingFragShader },
                { pathtracingSetLayout },
                pathtracingPipeline,
                VK_FORMAT_R32G32B32A32_SFLOAT
            );
            engine.destroyGraphicsPipeline(pathtracingPipeline);
            pathtracingPipeline = newPipeline;
        } else {
            pathtracingPipeline = engine.initGraphicsPipeline(
                vertexInput.get(),
                { vertShader, pathtracingFragShader },
                { pathtracingSetLayout },
                GraphicsPipeline(),
                VK_FORMAT_R32G32B32A32_SFLOAT
            );
        }
    }

    // Build the compositing pipeline
    {
        try {
            compositingFragShader = engine.initShader(VK_SHADER_STAGE_FRAGMENT_BIT, compositingFragShaderPath);
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
                { vertShader, compositingFragShader },
                { compositingSetLayout },
                compositingPipeline,
                VK_FORMAT_R32G32B32A32_SFLOAT
            );
            engine.destroyGraphicsPipeline(compositingPipeline);
            compositingPipeline = newCompositingPipeline;
        } else {
            compositingPipeline = engine.initGraphicsPipeline(
                vertexInput.get(),
                { vertShader, compositingFragShader },
                { compositingSetLayout },
                GraphicsPipeline(),
                VK_FORMAT_R32G32B32A32_SFLOAT
            );
        }
    }

    // Build the display pipeline
    {
        try {
            displayFragShader = engine.initShader(VK_SHADER_STAGE_FRAGMENT_BIT, displayFragShaderPath);
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
                { vertShader, displayFragShader },
                { displaySetLayout },
                displayPipeline
            );
            engine.destroyGraphicsPipeline(displayPipeline);
            displayPipeline = newDisplayPipeline;
        } else {
            displayPipeline = engine.initGraphicsPipeline(
                vertexInput.get(),
                { vertShader, displayFragShader },
                { displaySetLayout }
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
    
    engine.beginFrame();
    
    ctx.scene->runOnRender(ctx);

    // Rebuild descriptor set
    if (ctx.scene->checkBufferUpdate()) {
        std::vector<std::pair<ImageView, Sampler> > pathtracingCombinedImageSampler = {
            { pathtracingImageViews[0], pathtracingSamplers[0] },
            { pathtracingImageViews[1], pathtracingSamplers[1] }
        };

        std::vector<bufferList_t> storageBuffers = ctx.scene->getBufferLists();
        for (size_t i = 0; i < 2; i++) {
            std::vector<void*> descriptors = { &pathtracingUniformBuffers, &pathtracingCombinedImageSampler[1-i], &pixelInfoBuffers };
            for (bufferList_t &buffers : storageBuffers) {
                descriptors.push_back(&buffers);
            }
            
            DescriptorSetAllocation newAllocation = engine.createDescriptorSetAllocation(pathtracingSetLayout, descriptors);
            if (newAllocation.isValide()) {
                engine.destroyDescriptorSetAllocation(pathtracingDescriptorSets[i]);
                pathtracingDescriptorSets[i] = std::move(newAllocation);
            }
        }
    }
    
    engine.fillBuffer(engine.getBuffer(pathtracingUniformBuffers), ctx.pathtracerUBO);
    engine.fillBuffer(engine.getBuffer(displayUniformBuffers), ctx.screenUBO);
    frame = (frame + 1) % 2;
    
    pathtracingPass(ctx);
    uiPass(ctx);

    engine.endFrame();

    exportService.handleSave(ctx);
}

void RenderHandler::pathtracingPass(AppContext& ctx) {
    VkSmol& engine = *ctx.engine;

    CommandBuffer commandBuffer = engine.beginRecordingRender();

    VkExtent2D extent = engine.getExtent();

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
    
    {   // Rendering
        engine.barrier(
            commandBuffer,
            pathtracingImages[1-frame].get(),
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_NONE, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        );
        engine.barrier(
            commandBuffer,
            pathtracingImages[frame].get(),
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_ACCESS_NONE, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        );
        engine.beginDynamicRenderer(
            commandBuffer,
            pathtracingImageViews[frame].get(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
            {{ 0.0f, 0.0f, 0.0f, 1.0f }}
        );
        
        // Bind current descriptor set
        pathtracingDescriptorSets[frame].get(engine.getFrame()).bind(commandBuffer, pathtracingPipeline.getLayout());
        
        pathtracingPipeline.bind(commandBuffer);

        vertexBuffer.bindVertex(commandBuffer);
        indexBuffer.bindIndex(commandBuffer, VK_INDEX_TYPE_UINT16);
        
        pathtracingPipeline.setViewport(commandBuffer, viewport);
        pathtracingPipeline.setScissor(commandBuffer, scissor);
        pathtracingPipeline.drawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()));

        engine.endDynamicRenderer(commandBuffer);
        engine.barrier(
            commandBuffer,
            pathtracingImages[frame].get(),
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        );
        engine.barrier(
            commandBuffer,
            pathtracingImages[1-frame].get(),
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_NONE, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        );

    }

    {   // Compositing
        engine.barrier(
            commandBuffer,
            outputImage.get(),
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_ACCESS_NONE, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        );
        engine.beginDynamicRenderer(
            commandBuffer,
            outputImageView.get(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
            {{ 0.0f, 0.0f, 0.0f, 1.0f }}
        );

        compositingDescriptorSets[frame].get(engine.getFrame()).bind(commandBuffer, compositingPipeline.getLayout());
        compositingPipeline.bind(commandBuffer);

        vertexBuffer.bindVertex(commandBuffer);
        indexBuffer.bindIndex(commandBuffer, VK_INDEX_TYPE_UINT16);

        compositingPipeline.setViewport(commandBuffer, viewport);
        compositingPipeline.setScissor(commandBuffer, scissor);
        compositingPipeline.drawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()));

        engine.endDynamicRenderer(commandBuffer);
        engine.barrier(
            commandBuffer,
            outputImage.get(),
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        );
    }

    exportService.handleCopy(ctx, commandBuffer, outputImage);

    {   // Display
        engine.barrier(
            commandBuffer,
            nullptr,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_ACCESS_NONE, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        );
        engine.beginDynamicRenderer(
            commandBuffer,
            nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
            {{ 1.0f, 0.0f, 1.0f, 1.0f }}
        );
        
        displayDescriptorSets[frame].get(engine.getFrame()).bind(commandBuffer, displayPipeline.getLayout());

        displayPipeline.bind(commandBuffer);

        vertexBuffer.bindVertex(commandBuffer);
        indexBuffer.bindIndex(commandBuffer, VK_INDEX_TYPE_UINT16);
        
        displayPipeline.setViewport(commandBuffer, viewport);
        displayPipeline.setScissor(commandBuffer, scissor);
        displayPipeline.drawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()));

        engine.endDynamicRenderer(commandBuffer);
        engine.barrier(
            commandBuffer,
            nullptr,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_NONE,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
        );
    }

    engine.endRecoringRender(commandBuffer);
}

void RenderHandler::uiPass(AppContext& ctx) {
    VkSmol& engine = *ctx.engine;

    CommandBuffer commandBuffer = engine.beginRecordingUiRender();
    
    // TODO: might set default barrier and dyamic rendering context (at least for the UI)
    engine.barrier(
        commandBuffer,
        nullptr,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_ACCESS_NONE, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    );
    engine.beginDynamicRenderer(
        commandBuffer,
        nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE,
        {{ 1.0f, 0.0f, 1.0f, 1.0f }}
    );
    
    ctx.ui->draw(commandBuffer, ctx);

    engine.endDynamicRenderer(commandBuffer);
    engine.barrier(
        commandBuffer,
        nullptr,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_NONE,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
    );

    engine.endRecoringUiRender(commandBuffer);
}
