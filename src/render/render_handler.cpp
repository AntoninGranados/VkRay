#include "render_handler.hpp"

#include "./../engine/engine.hpp"
#include "./../scene/scene.hpp"
#include "./../camera.hpp"
#include "./../notification_handler.hpp"
#include "./../ui_handler.hpp"
#include "./../parameter_handler.hpp"
#include "./../animation_handler.hpp"

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
        screenUniformBuffers = engine.initBufferList(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(ScreenUBO));

        VkExtent2D extent = engine.getExtent();
        size_t pixelInfoBytes = static_cast<size_t>(extent.width) * extent.height * sizeof(PixelInfo);
        pixelInfoBuffers = engine.initSharedBufferList(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, pixelInfoBytes);
    }

    {   // Image (image + view + sampler) creation
        VkExtent2D extent = engine.getExtent();
        for (size_t i = 0; i < 2; i++) {
            images[i] = engine.initImage(
                extent.width, extent.height,
                VK_FORMAT_R32G32B32A32_SFLOAT,  // Use 32 bit float format to avoid quantizing every accumulation step
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            );
            imageViews[i] = engine.initImageView(images[i]);
            samplers[i] = engine.initSampler();
        }

        exportService.init(engine, extent.width, extent.height);
    }

    setLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    setLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    setLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    for (size_t i = 0; i < ctx.scene->getBufferLists().size(); i++) {
        setLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    }
    engine.initDescriptorSetLayout(setLayout);
    
    screenSetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    screenSetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    screenSetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    engine.initDescriptorSetLayout(screenSetLayout);
    
    {   // Pipeline creation
        buildPipeline(ctx);
        
        Shader screenVertShader = engine.initShader(VK_SHADER_STAGE_VERTEX_BIT,   "./shaders/vert.glsl");
        Shader screenFragShader = engine.initShader(VK_SHADER_STAGE_FRAGMENT_BIT, "./shaders/frag.glsl");
        
        VertexInput<ScreenVertex> screenVertexInput;
        screenVertexInput.addAttributeDescription(VK_FORMAT_R32G32_SFLOAT, offsetof(ScreenVertex, pos));
        screenPipeline = engine.initGraphicsPipeline(
            screenVertexInput.get(),
            { screenVertShader, screenFragShader },
            { screenSetLayout }
        );
        
        engine.destroyShader(screenVertShader);
        engine.destroyShader(screenFragShader);
    }

    {   // Descriptor sets creation
        std::vector<std::pair<ImageView, Sampler> > combinedImageSampler = {
            { imageViews[0], samplers[0] },
            { imageViews[1], samplers[1] }
        };
        
        std::vector<bufferList_t> storageBuffers = ctx.scene->getBufferLists();
        for (size_t i = 0; i < 2; i++) {
            std::vector<void*> descriptors = { &pathtracingUniformBuffers, &combinedImageSampler[1-i], &pixelInfoBuffers };
            for (bufferList_t &buffers : storageBuffers) {
                descriptors.push_back(&buffers);
            }

            descriptorSets[i] = engine.initDescriptorSetList(setLayout, descriptors);

            screenDescriptorSets[i] = engine.initDescriptorSetList(
                screenSetLayout,
                { &combinedImageSampler[i], &screenUniformBuffers, &pixelInfoBuffers }
            );
        }
    }
}

void RenderHandler::destroy(AppContext& ctx) {
    VkSmol& engine = *ctx.engine;

    engine.destroyDescriptorSetLayout(setLayout);
    engine.destroyDescriptorSetLayout(screenSetLayout);

    for (size_t i = 0; i < 2; i++) {
        engine.destroySampler(samplers[i]);
        engine.destroyImage(images[i]);
        engine.destroyImageView(imageViews[i]);
    }

    engine.destroyBuffer(vertexBuffer);
    engine.destroyBuffer(indexBuffer);
    engine.destroyBufferList(pathtracingUniformBuffers);
    engine.destroyBufferList(screenUniformBuffers);
    engine.destroyBufferList(pixelInfoBuffers);
    exportService.destroy(engine);
    
    engine.destroyGraphicsPipeline(pipeline);
    engine.destroyGraphicsPipeline(screenPipeline);
}

void RenderHandler::buildPipeline(AppContext& ctx) {
    VkSmol& engine = *ctx.engine;
    NotificationHandler& notifications = *ctx.notifications;

    engine.waitIdle();

    std::string vertShaderPath = "./shaders/vert.glsl";
    Shader vertShader;
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
    
    std::string fragShaderPath = "./shaders/raytracing/raytracing.glsl";
    Shader fragShader;
    try {
        fragShader = engine.initShader(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderPath);
    } catch (...) {
        engine.destroyShader(vertShader);
        std::cerr << "[ERROR] Failed to compile shader [" << fragShaderPath << "]: pipeline not built" << std::endl;
        notifications.pushMessage(
            NotificationType::Error,
            "Failed to compile shader [" + fragShaderPath + "]: pipeline not built"
        );
        return;
    }

    VertexInput<ScreenVertex> vertexInput;
    vertexInput.addAttributeDescription(VK_FORMAT_R32G32_SFLOAT, offsetof(ScreenVertex, pos));

    if (pipeline.get() != VK_NULL_HANDLE) {
        GraphicsPipeline newPipeline = engine.initGraphicsPipeline(
            vertexInput.get(),
            { vertShader, fragShader },
            { setLayout },
            pipeline,
            VK_FORMAT_R32G32B32A32_SFLOAT
        );
        engine.destroyGraphicsPipeline(pipeline);
        pipeline = newPipeline;
    } else {
        pipeline = engine.initGraphicsPipeline(
            vertexInput.get(),
            { vertShader, fragShader },
            { setLayout },
            GraphicsPipeline(),
            VK_FORMAT_R32G32B32A32_SFLOAT
        );
    }

    engine.destroyShader(vertShader);
    engine.destroyShader(fragShader);

    std::cout << "[INFO] Built the main pipeline by recompiling [" << vertShaderPath << "] and [" << fragShaderPath << "]" << std::endl;
    notifications.pushMessage(
        NotificationType::Info,
        "(Re)Built the main pipeline"
    );
}


void RenderHandler::render(AppContext& ctx) {
    VkSmol& engine = *ctx.engine;

    uint64_t renderSamplesPerPixel = ctx.parameters->getInt("renderSamples");
    if (ctx.renderState->renderMode != RenderMode::Preview && renderSamplesPerPixel > 0 && !ctx.renderState->pendingExit && !(*ctx.restartRender)) {
        if (ctx.renderState->sampleCount >= renderSamplesPerPixel) {
            // renderOutput.requested = true;
            exportService.requestSave();
            ctx.renderState->pendingExit = ctx.renderState->renderMode == RenderMode::RenderSingle; // we don't want to exist when rendering an animation
        }
    }
    
    engine.beginFrame();
    
    ctx.scene->runOnRender(ctx);

    // Rebuild descriptor set
    if (ctx.scene->checkBufferUpdate()) {
        std::vector<std::pair<ImageView, Sampler> > combinedImageSampler = {
            { imageViews[0], samplers[0] },
            { imageViews[1], samplers[1] }
        };

        std::vector<bufferList_t> storageBuffers = ctx.scene->getBufferLists();
        for (size_t i = 0; i < 2; i++) {
            std::vector<void*> descriptors = { &pathtracingUniformBuffers, &combinedImageSampler[1-i], &pixelInfoBuffers };
            for (bufferList_t &buffers : storageBuffers) {
                descriptors.push_back(&buffers);
            }
            
            descriptorSets[i] = engine.initDescriptorSetList(setLayout, descriptors);
        }
    }
    
    engine.fillBuffer(engine.getBuffer(pathtracingUniformBuffers), ctx.pathtracerUBO);
    engine.fillBuffer(engine.getBuffer(screenUniformBuffers), ctx.screenUBO);
    frame = (frame + 1) % 2;
    
    renderMain(ctx);
    renderUiLayer(ctx);

    engine.endFrame();

    exportService.handleSave(ctx);
}

void RenderHandler::renderMain(AppContext& ctx) {
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
            images[1-frame].get(),
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_NONE, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        );
        engine.barrier(
            commandBuffer,
            images[frame].get(),
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_ACCESS_NONE, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        );
        engine.beginDynamicRenderer(
            commandBuffer,
            imageViews[frame].get(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
            {{ 0.0f, 0.0f, 0.0f, 1.0f }}
        );
        
        // Bind current descriptor set
        engine.getDescriptorSet(descriptorSets[frame]).bind(commandBuffer, pipeline.getLayout());
        
        pipeline.bind(commandBuffer);

        vertexBuffer.bindVertex(commandBuffer);
        indexBuffer.bindIndex(commandBuffer, VK_INDEX_TYPE_UINT16);
        
        pipeline.setViewport(commandBuffer, viewport);
        pipeline.setScissor(commandBuffer, scissor);
        pipeline.drawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()));

        engine.endDynamicRenderer(commandBuffer);
        engine.barrier(
            commandBuffer,
            images[frame].get(),
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        );
        engine.barrier(
            commandBuffer,
            images[1-frame].get(),
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_NONE, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        );

        exportService.handleCopy(ctx, commandBuffer, images[frame]);
    }

    {   // Screen
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
        
        engine.getDescriptorSet(screenDescriptorSets[frame]).bind(commandBuffer, screenPipeline.getLayout());

        screenPipeline.bind(commandBuffer);

        vertexBuffer.bindVertex(commandBuffer);
        indexBuffer.bindIndex(commandBuffer, VK_INDEX_TYPE_UINT16);
        
        screenPipeline.setViewport(commandBuffer, viewport);
        screenPipeline.setScissor(commandBuffer, scissor);
        screenPipeline.drawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()));

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

void RenderHandler::renderUiLayer(AppContext& ctx) {
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
