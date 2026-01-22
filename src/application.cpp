#include "application.hpp"

#include <algorithm>
#include <cmath>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
#include <stb_image_write.h>

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

const float PREVIEW_VIEWPORT_SCALE = 0.8f;

const std::vector<ScreenVertex> vertices = {
    { .position={ 1.0f, 1.0f} },
    { .position={ 1.0f,-1.0f} },
    { .position={-1.0f,-1.0f} },
    { .position={-1.0f, 1.0f} }
};

const std::vector<index_t> indices = {
    0, 1, 2, 2, 3, 0
};

static std::string buildScreenshotPath() {
    auto now = std::chrono::system_clock::now();
    auto nowSecs = std::chrono::time_point_cast<std::chrono::seconds>(now);
    auto value = nowSecs.time_since_epoch().count();
    return "screenshot_" + std::to_string(value) + ".png";
}

NotificationManager Application::notificationManager;

Application::Application() {
    engine.init("VkRay", VK_MAKE_API_VERSION(0, 1, 0, 0));

    glfwSetWindowAttrib(engine.getWindow().get(), GLFW_RESIZABLE, GLFW_FALSE);
    glfwSetCursorPosCallback(
        engine.getWindow().get(),
        [](GLFWwindow* window, double x, double y) {
            ImGui_ImplGlfw_CursorPosCallback(window, x, y);
            auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
            const bool cameraLocked = app->camera.isLocked();
            if (cameraLocked && (ImGui::GetIO().WantCaptureMouse || app->ui.capturesMouse || ImGuizmo::IsUsing()))
                return;
            if (app->camera.cursorPosCallback(window, x, y))
                app->restartRender = true;
        }
    );
    glfwSetScrollCallback(
        engine.getWindow().get(), 
        [](GLFWwindow* window, double xoffset, double yoffset) {
            ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
            auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
            if (ImGui::GetIO().WantCaptureMouse || app->ui.capturesMouse) return;
            if (app->camera.scrollCallback(window, xoffset, yoffset))
                app->restartRender = true;
        }
    );

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
    
        raytracingUniformBuffers = engine.initBufferList(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(RaytracingUBO));
        screenUniformBuffers = engine.initBufferList(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(ScreenUBO));

        VkExtent2D extent = engine.getExtent();
        size_t pixelInfoBytes = static_cast<size_t>(extent.width) * extent.height * 4 * sizeof(float);
        pixelInfoBuffers = engine.initSharedBufferList(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, pixelInfoBytes);
    }

    {   // Image (image + view + sampler) creation
        VkExtent2D extent = engine.getExtent();
        for (size_t i = 0; i < 2; i++) {
            images[i] = engine.initImage(
                extent.width, extent.height,
                // Use float format to avoid quantizing every accumulation step
                VK_FORMAT_R32G32B32A32_SFLOAT,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            );
            imageViews[i] = engine.initImageView(images[i]);
            samplers[i] = engine.initSampler();
        }
        screenshotWidth = extent.width;
        screenshotHeight = extent.height;
        screenshotBuffer = engine.initReadbackBuffer(static_cast<size_t>(screenshotWidth) * screenshotHeight * 4 * sizeof(float));
    }
    
    initParameters();
    initScene();

    setLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    setLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    setLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    for (size_t i = 0; i < scene.getBufferLists().size(); i++) {
        setLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    }
    engine.initDescriptorSetLayout(setLayout);
    
    screenSetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    screenSetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    screenSetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    engine.initDescriptorSetLayout(screenSetLayout);
    
    {   // Pipeline creation
        rebuildPipeline();
        
        Shader screenVertShader = engine.initShader(VK_SHADER_STAGE_VERTEX_BIT,   "./res/shader/vert.glsl");
        Shader screenFragShader = engine.initShader(VK_SHADER_STAGE_FRAGMENT_BIT, "./res/shader/frag.glsl");
        
        VertexInput<ScreenVertex> screenVertexInput;
        screenVertexInput.addAttributeDescription(VK_FORMAT_R32G32_SFLOAT, offsetof(ScreenVertex, position));
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
        
        std::vector<bufferList_t> storageBuffers = scene.getBufferLists();
        for (size_t i = 0; i < 2; i++) {
            std::vector<void*> descriptors = { &raytracingUniformBuffers, &combinedImageSampler[1-i], &pixelInfoBuffers };
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


Application::~Application() {
    engine.waitIdle();

    engine.destroyDescriptorSetLayout(setLayout);
    engine.destroyDescriptorSetLayout(screenSetLayout);

    for (size_t i = 0; i < 2; i++) {
        engine.destroySampler(samplers[i]);
        engine.destroyImage(images[i]);
        engine.destroyImageView(imageViews[i]);
    }

    engine.destroyBuffer(vertexBuffer);
    engine.destroyBuffer(indexBuffer);
    engine.destroyBufferList(raytracingUniformBuffers);
    engine.destroyBufferList(screenUniformBuffers);
    engine.destroyBufferList(pixelInfoBuffers);
    engine.destroyBuffer(screenshotBuffer);
    scene.destroy(engine);
    
    engine.destroyGraphicsPipeline(pipeline);
    engine.destroyGraphicsPipeline(screenPipeline);
    
    engine.terminate();
}


void Application::initScene() {
    scene.init(engine);

    scene.setMessageCallback([](NotificationType type, std::string content) {
        Application::notificationManager.pushMessage(type, content);
    });
    scene.setPreviewCameraCallback([this](const CameraHandle &handle) {
                if (previewCameraHandle && scene.containsObject(previewCameraHandle))
            previewCameraHandle->setPreview(false);
        float dist = glm::length(camera.getTarget() - camera.getPosition());
        if (dist < 0.1f) dist = 1.0f;
        camera.setPosition(handle.getPosition());
        camera.setTarget(handle.getPosition() + handle.getDirection() * dist);
        float fovRad = glm::radians(handle.getFov());
        float previewFovRad = 2.0f * atanf(tanf(fovRad * 0.5f) / PREVIEW_VIEWPORT_SCALE);
        camera.setFov(glm::degrees(previewFovRad));
        camera.setAperture(handle.getAperture());
        camera.setFocusDepth(handle.getFocusDepth());
        previewCameraHandle = const_cast<CameraHandle*>(&handle);
        previewCameraHandle->setPreview(true);
        restartRender = true;
    });

    LightMode mode = parameters.getEnum<LightMode>("lightMode");
    initEmpty(engine, scene, mode);
}

void Application::initParameters() {
    parameters.addInt("maxBounces", "Max Bounces", 8, 1, 20, 1, false, "Pathtracer");
    parameters.addInt("previewSamples", "Preview Samples", 1, 1, 10, 1, false, "Pathtracer");
    parameters.addInt("renderSamples", "Render Samples", 2048, 1, 4096, 1, false, "Pathtracer");
    parameters.addFloat("movingResolution", "Moving Resolution", 8.0f, 1.0f, 50.0f, 1.0f, false, "Pathtracer");
    parameters.addFloat("previewResolution", "Preview Resolution", 1.0f, 1.0f, 50.0f, 1.0f, true, "Pathtracer");
    parameters.addFloat("renderResolution", "Render Resolution", 1.0f, 1.0f, 50.0f, 1.0f, false, "Pathtracer");
    parameters.addBool("importanceSampling", "Importance Sampling", true, false, "Pathtracer");
    parameters.addBool("varianceSampling", "Variance Sampling", true, false, "Pathtracer");
    parameters.addInt("varianceWarmup", "Variance Warmup Samples", 64, 0, 2048, 1, false, "Pathtracer");
    parameters.addEnum(
        "debugView",
        "Debug View",
        static_cast<int>(DebugView::None),
        { "None", "Bounces", "Normal", "Selection Mask", "Variance" },
        true,
        "Pathtracer"
    );

    parameters.addEnum(
        "lightMode",
        "Light Mode",
        static_cast<int>(LightMode::Day),
        { "Day", "Sunset", "Night", "Empty" },
        true,
        "Scene"
    );
}


void Application::run() {
    auto startTime = std::chrono::high_resolution_clock::now();

    while(!engine.shouldTerminate() && !shouldClose) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        startTime = currentTime;
        
        CommandBuffer commandBuffer;
        
        engine.beginFrame();
        
        onFrameStart(deltaTime);

        commandBuffer = engine.beginRecordingRender();
        {
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
                if (screenshotRequested) {
                    copyImageToScreenshotBuffer(commandBuffer, images[frame]);
                    screenshotPendingSave = true;
                    screenshotRequested = false;
                }
                engine.barrier(
                    commandBuffer,
                    images[1-frame].get(),
                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_ACCESS_NONE, VK_ACCESS_SHADER_READ_BIT,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
                );
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
                    {{ 0.0f, 0.0f, 0.0f, 1.0f }}
                );
                
                engine.fillBuffer(engine.getBuffer(screenUniformBuffers), &screenUBO);
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
        }
        engine.endRecoringRender(commandBuffer);
            
        // TODO: might set default barrier and dyamic rendering context (at least for the UI)
        commandBuffer = engine.beginRecordingUiRender();
        {
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
            
            drawUI(commandBuffer);

            engine.endDynamicRenderer(commandBuffer);
            engine.barrier(
                commandBuffer,
                nullptr,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_NONE,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
            );
        }
        engine.endRecoringUiRender(commandBuffer);

        engine.endFrame();

        if (screenshotPendingSave) {
            engine.waitIdle();
            saveScreenshotBuffer(buildScreenshotPath());
            screenshotPendingSave = false;
            if (renderModePendingExit) {
                renderMode = false;
                renderModePendingExit = false;
                ui.toggled = ui.toggledBeforeRender;
                samplesPerSecEMA = 0.0;
                samplesPerSecInitialized = false;
                samplesPerSecAccumTime = 0.0;
                samplesPerSecAccumSamples = 0.0;
            }
        }
    }
}


void Application::onFrameStart(float dt) {
    fillUBOs(raytracingUBO, screenUBO);
    engine.fillBuffer(engine.getBuffer(raytracingUniformBuffers), &raytracingUBO);
    scene.fillBuffers(engine);
    prevResolution = resolution;

    // Rebuild descriptor set
    if (scene.checkBufferUpdate()) {
        std::vector<std::pair<ImageView, Sampler> > combinedImageSampler = {
            { imageViews[0], samplers[0] },
            { imageViews[1], samplers[1] }
        };

        std::vector<bufferList_t> storageBuffers = scene.getBufferLists();
        for (size_t i = 0; i < 2; i++) {
            std::vector<void*> descriptors = { &raytracingUniformBuffers, &combinedImageSampler[1-i], &pixelInfoBuffers };
            for (bufferList_t &buffers : storageBuffers) {
                descriptors.push_back(&buffers);
            }
            
            descriptorSets[i] = engine.initDescriptorSetList(setLayout, descriptors);
        }
    }
    
    frame = (frame + 1) % 2;
    frameCount++;
    sampleCount += static_cast<uint64_t>(parameters.getInt("previewSamples"));

    handleInput(dt);

    if (previewCameraHandle) {
                if (!scene.containsObject(previewCameraHandle)) {
            previewCameraHandle = nullptr;
        } else {
            previewCameraHandle->setPosition(camera.getPosition());
            previewCameraHandle->setDirection(camera.getDirection());
            float fovRad = glm::radians(camera.getFov());
            float handleFovRad = 2.0f * atanf(tanf(fovRad * 0.5f) * PREVIEW_VIEWPORT_SCALE);
            previewCameraHandle->setFov(glm::max(1.0f, glm::degrees(handleFovRad)));
            previewCameraHandle->setAperture(camera.getAperture());
            previewCameraHandle->setFocusDepth(camera.getFocusDepth());
        }
    }
    
    if (notificationManager.isCommandRequested(Command::Exit)) {
        shouldClose = true;
    } if (notificationManager.isCommandRequested(Command::Render)) {
        if (!renderMode) {
            scene.clearSelection();
            ui.toggledBeforeRender = ui.toggled;
            ui.toggled = false;
            renderMode = true;
            renderModePendingExit = false;
            restartRender = true;
            samplesPerSecEMA = 0.0;
            samplesPerSecInitialized = false;
            samplesPerSecAccumTime = 0.0;
            samplesPerSecAccumSamples = 0.0;
        }
    } if (notificationManager.isCommandRequested(Command::Reload)) {
        rebuildPipeline();
        restartRender = true;
    } if (notificationManager.isCommandRequested(Command::Screenshot)) {
        screenshotRequested = true;
    }

    int renderSamplesPerPixel = parameters.getInt("renderSamples");
    if (renderMode && renderSamplesPerPixel > 0 && !renderModePendingExit && !restartRender) {
        if (sampleCount >= static_cast<uint64_t>(renderSamplesPerPixel)) {
            screenshotRequested = true;
            renderModePendingExit = true;
        }
    }

    if (restartRender) {
        frameCount = 1;
        sampleCount = 0;
        restartRender = false;
        if (!renderMode) resolution = parameters.getFloat("movingResolution");
    }
}

void Application::drawUI(CommandBuffer commandBuffer) {
    if (!ui.toggled && !renderMode) return;

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    
    ImGui::NewFrame();
    updateUiState();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding = 5.0f;

    if (renderMode) {
        drawRenderUi();
    } else {
        drawMainUi();
    }

    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer.get());

}

void Application::updateUiState() {
    ImGuiIO& io = ImGui::GetIO();
    ui.capturesMouse = io.WantCaptureMouse;
    ui.capturesKeyboard = io.WantCaptureKeyboard;
}

void Application::drawRenderUi() {
    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowBgAlpha(0.6f);
    ImGui::Begin("Loading",
        nullptr,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration
    );
    int renderSamplesPerPixel = parameters.getInt("renderSamples");
    if (renderSamplesPerPixel > 0) {
        float progress = static_cast<float>(std::min<uint64_t>(sampleCount, renderSamplesPerPixel))
            / static_cast<float>(renderSamplesPerPixel);
        char overlay[64];
        snprintf(
            overlay,
            sizeof(overlay),
            "%llu / %d",
            static_cast<unsigned long long>(std::min<uint64_t>(sampleCount, renderSamplesPerPixel)),
            renderSamplesPerPixel
        );
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.55f, 0.55f, 0.55f, 0.85f));
        ImGui::ProgressBar(progress, ImVec2(ImGui::GetContentRegionAvail().x, 0.0f), "");
        ImGui::PopStyleColor();

        ImVec2 textSize = ImGui::CalcTextSize(overlay);
        ImVec2 barMin = ImGui::GetItemRectMin();
        ImVec2 barMax = ImGui::GetItemRectMax();
        ImVec2 textPos(
            (barMin.x + barMax.x - textSize.x) * 0.5f,
            (barMin.y + barMax.y - textSize.y) * 0.5f
        );
        ImGui::GetWindowDrawList()->AddText(textPos, ImGui::GetColorU32(ImGuiCol_Text), overlay);
    }
    float samplesPerSec = static_cast<float>(samplesPerSecEMA);
    ImGui::Text("%.1f samples/sec", samplesPerSec);
    if (renderSamplesPerPixel > 0 && samplesPerSec > 0.0f) {
        uint64_t remaining = 0;
        if (sampleCount < static_cast<uint64_t>(renderSamplesPerPixel)) {
            remaining = static_cast<uint64_t>(renderSamplesPerPixel) - sampleCount;
        }
        float etaSec = static_cast<float>(remaining) / samplesPerSec;
        int etaMin = static_cast<int>(etaSec / 60.0f);
        int etaRemSec = static_cast<int>(etaSec) % 60;
        ImGui::Text("ETA: %dm %02ds", etaMin, etaRemSec);
    } else {
        ImGui::Text("ETA: --");
    }
    ImGui::End();
}

void Application::drawMainUi() {
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::AllowAxisFlip(false);
    ImGuizmo::BeginFrame();
    
    ImGuiID dockspace_id = ImGui::GetID("Dock space");
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode;
    ImGui::DockSpaceOverViewport(dockspace_id, ImGui::GetMainViewport(), dockspaceFlags);

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("Gizmo View", nullptr, ImGuiWindowFlags_NoBackground | 
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoMouseInputs |
        ImGuiWindowFlags_NoDocking
    );
    {
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 windowSize = ImGui::GetWindowSize();
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(windowPos.x, windowPos.y, windowSize.x, windowSize.y);

        scene.drawGuizmo(
            camera.getView(),
            camera.getProjection(engine.getWindow().get())
        );
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowBgAlpha(0.6f);
    ImGui::Begin("FPS",
        nullptr,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration
    );
    {
        ImGui::Text("%.1f fps (%.3f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
        ImGui::Text("%llu samples", static_cast<unsigned long long>(sampleCount));
        ImGui::Text("%.0f samples/sec", ImGui::GetIO().Framerate * parameters.getInt("previewSamples"));
    }
    ImGui::End();

    ImGui::SetNextWindowBgAlpha(0.6f);
    ImGui::Begin("Information", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    {
        if (ImGui::CollapsingHeader("Camera")) {
            camera.drawPreviewUI(restartRender);
        }
        
        if (ImGui::CollapsingHeader("Pathtracer")) {
            parameters.drawGroup("Pathtracer", restartRender);
        }
        
        if (ImGui::CollapsingHeader("Scene")) {
            parameters.drawGroup("Scene", restartRender);
            if (ImGui::Button("Load Scene Preset", { -FLT_MIN, 0 }) && !ImGui::IsPopupOpen("Scene Preset")) {
                ImGui::OpenPopup("Scene Preset");
            }
            scene.drawUI(engine);
        }

        if (ImGui::BeginPopupModal("Scene Preset", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
            LightMode mode = parameters.getEnum<LightMode>("lightMode");
            if (ImGui::Button("Empty", { 200, 0 })) {
                initEmpty(engine, scene, mode);
                restartRender = true;
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::Button("Suzanne", { 200, 0 })) {
                initSuzanne(engine, scene, mode);
                restartRender = true;
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::Button("Sponza", { 200, 0 })) {
                initSponza(engine, scene, mode);
                restartRender = true;
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::Button("Cornell Box", { 200, 0 })) {
                initCornellBox(engine, scene, mode);
                restartRender = true;
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::Button("Random Spheres", { 200, 0 })) {
                initRandomSpheres(engine, scene, mode);
                restartRender = true;
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::PushStyleColor(ImGuiCol_Button, { 1.0, 0.03, 0.0, 1.0 });
            if (ImGui::Button("Cancel", { 200, 0 })) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleColor();
            
            ImGui::EndPopup();
        }
    }
    ImGui::End();
    
    scene.drawSelectedUI(engine);
    notificationManager.drawNotifications();

    syncPreviewCameraFromHandle();
}

void Application::syncPreviewCameraFromHandle() {
    if (!previewCameraHandle)
        return;
    if (!scene.containsObject(previewCameraHandle)) {
        previewCameraHandle = nullptr;
        return;
    }

        float dist = glm::length(camera.getTarget() - camera.getPosition());
    if (dist < 0.1f) dist = 1.0f;
    camera.setPosition(previewCameraHandle->getPosition());
    camera.setTarget(previewCameraHandle->getPosition() + previewCameraHandle->getDirection() * dist);
    float fovRad = glm::radians(previewCameraHandle->getFov());
    float previewFovRad = 2.0f * atanf(tanf(fovRad * 0.5f) / PREVIEW_VIEWPORT_SCALE);
    camera.setFov(glm::degrees(previewFovRad));
    camera.setAperture(previewCameraHandle->getAperture());
    camera.setFocusDepth(previewCameraHandle->getFocusDepth());
}


void Application::handleInput(float dt) {
    if (renderMode)
        handleInputRender(dt);
    else
        handleInputPreview(dt);
}

void Application::handleInputRender(float dt) {
    resolution = parameters.getFloat("renderResolution");

    double dtSafe = std::max(static_cast<double>(dt), 0.0);
    samplesPerSecAccumTime += dtSafe;
    samplesPerSecAccumSamples += static_cast<double>(parameters.getInt("previewSamples"));
    if (samplesPerSecAccumTime >= 1.0) {
        double instant = samplesPerSecAccumSamples / std::max(samplesPerSecAccumTime, 1e-6);
        double alpha = 1.0 - std::exp(-samplesPerSecAccumTime / 5.0);
        if (!samplesPerSecInitialized) {
            samplesPerSecEMA = instant;
            samplesPerSecInitialized = true;
        } else {
            samplesPerSecEMA += alpha * (instant - samplesPerSecEMA);
        }
        samplesPerSecAccumTime = 0.0;
        samplesPerSecAccumSamples = 0.0;
    }

    glfwSetInputMode(engine.getWindow().get(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    if (glfwGetKey(engine.getWindow().get(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        renderMode = false;
        renderModePendingExit = false;
        ui.toggled = ui.toggledBeforeRender;
        samplesPerSecEMA = 0.0;
        samplesPerSecInitialized = false;
        samplesPerSecAccumTime = 0.0;
        samplesPerSecAccumSamples = 0.0;
    }
}

void Application::handleInputPreview(float dt) {
    resolution = parameters.getFloat("previewResolution");
    
    const bool blockMouseInput = ImGuizmo::IsUsing() || (camera.isLocked() && (ui.capturesMouse || ImGui::GetIO().WantCaptureMouse));
    const bool blockKeyboardInput = ui.capturesKeyboard || ImGui::GetIO().WantCaptureKeyboard;

    const bool middleDown = glfwGetMouseButton(engine.getWindow().get(), GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
    if (!blockMouseInput && middleDown && !ui.middleClickWasDown) {
        double xpos, ypos;
        glfwGetCursorPos(engine.getWindow().get(), &xpos, &ypos);
        int width, height;
        glfwGetWindowSize(engine.getWindow().get(), &width, &height);
        float dist;
        glm::vec3 p;
        if (scene.raycast({ xpos, ypos }, { static_cast<float>(width), static_cast<float>(height) }, camera, dist, p, false, false)) {
            camera.setFocusDepth(dist);
            if (previewCameraHandle)
                previewCameraHandle->setFocusDepth(dist);
            restartRender = true;
        }
    }
    ui.middleClickWasDown = middleDown;

    if (!blockMouseInput && glfwGetMouseButton(engine.getWindow().get(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(engine.getWindow().get(), &xpos, &ypos);
        int width, height;
        glfwGetWindowSize(engine.getWindow().get(), &width, &height);
        float dist;
        glm::vec3 p;
        scene.raycast({ xpos, ypos }, { static_cast<float>(width), static_cast<float>(height) }, camera, dist, p, true);
    }
    
    if (glfwGetKey(engine.getWindow().get(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        if (!ui.toggled) ui.toggled = true;
        else scene.clearSelection();
    }
    
    if (!blockKeyboardInput && camera.processInput(engine.getWindow().get(), dt))
    restartRender = true;
    
    if (camera.isLocked() || blockMouseInput)
        glfwSetInputMode(engine.getWindow().get(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    else
        glfwSetInputMode(engine.getWindow().get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    if (!blockKeyboardInput && glfwGetKey(engine.getWindow().get(), GLFW_KEY_R) == GLFW_PRESS)
    restartRender = true;

    if (glfwGetKey(engine.getWindow().get(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        if (previewCameraHandle)
            previewCameraHandle->setPreview(false);
        previewCameraHandle = nullptr;
    }
    
    if (scene.checkUpdate()) 
    restartRender = true;
}


void Application::fillUBOs(RaytracingUBO &raytracingUBO, ScreenUBO &screenUBO) {
    // Raytracing UBO
    raytracingUBO.cameraPos = camera.getPosition();
    raytracingUBO.cameraDir = camera.getDirection();
    raytracingUBO.tanHFov = camera.getTanHFov();
    raytracingUBO.aperture = camera.getAperture();
    raytracingUBO.focusDepth = camera.getFocusDepth();

    VkExtent2D extent = engine.getExtent();
    raytracingUBO.screenSize = { (float)extent.width, (float)extent.height };
    raytracingUBO.aspect = raytracingUBO.screenSize.x / raytracingUBO.screenSize.y;
    raytracingUBO.resolution = resolution;
    raytracingUBO.prevResolution = prevResolution;

    if (frameCount <= 1)
        lastTime = glfwGetTime();
    raytracingUBO.frameCount = frameCount;
    raytracingUBO.time = glfwGetTime() - lastTime;
    
    raytracingUBO.lightMode = parameters.getEnum<LightMode>("lightMode");

    raytracingUBO.maxBounces = parameters.getInt("maxBounces");
    raytracingUBO.samplesPerPixel = parameters.getInt("previewSamples");
    raytracingUBO.importanceSampling = static_cast<int>(parameters.getBool("importanceSampling"));
    raytracingUBO.varianceSampling = static_cast<int>(parameters.getBool("varianceSampling"));
    raytracingUBO.varianceWarmupSamples = parameters.getInt("varianceWarmup");
    raytracingUBO.debugView = static_cast<int>(parameters.getEnum<DebugView>("debugView"));

    // Screen UBO
    screenUBO.frameCount = frameCount;
    screenUBO.resolution = resolution;
    screenUBO.debugView = static_cast<int>(parameters.getEnum<DebugView>("debugView"));
    screenUBO.previewBorderEnabled = previewCameraHandle ? 1 : 0;
}

// TODO: make this function asynchronous ?
void Application::rebuildPipeline() {
    engine.waitIdle();

    std::string vertShaderPath = "./res/shader/vert.glsl";
    Shader vertShader;
    try {
        vertShader = engine.initShader(VK_SHADER_STAGE_VERTEX_BIT, vertShaderPath);
    } catch (...) {
        std::cerr << "[ERROR] Failed to compile shader [" << vertShaderPath << "]: pipeline not built" << std::endl;
        notificationManager.pushMessage(
            NotificationType::Error,
            "Failed to compile shader [" + vertShaderPath + "]: pipeline not built"
        );
        return;
    }
    
    std::string fragShaderPath = "./res/shader/raytracing/raytracing.glsl";
    Shader fragShader;
    try {
        fragShader = engine.initShader(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderPath);
    } catch (...) {
        engine.destroyShader(vertShader);
        std::cerr << "[ERROR] Failed to compile shader [" << fragShaderPath << "]: pipeline not built" << std::endl;
        notificationManager.pushMessage(
            NotificationType::Error,
            "Failed to compile shader [" + fragShaderPath + "]: pipeline not built"
        );
        return;
    }

    VertexInput<ScreenVertex> vertexInput;
    vertexInput.addAttributeDescription(VK_FORMAT_R32G32_SFLOAT, offsetof(ScreenVertex, position));

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
    notificationManager.pushMessage(
        NotificationType::Info,
        "(Re)Built the main pipeline"
    );
}

void Application::copyImageToScreenshotBuffer(CommandBuffer commandBuffer, Image image) {
    engine.barrier(
        commandBuffer,
        image.get(),
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_READ_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
    );
    image.copyToBuffer(commandBuffer, screenshotBuffer);
    engine.barrier(
        commandBuffer,
        image.get(),
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
    );
}

void Application::saveScreenshotBuffer(std::string path) {
    size_t floatCount = static_cast<size_t>(screenshotWidth) * screenshotHeight * 4;
    size_t byteCount = floatCount * sizeof(float);
    std::vector<float> floatPixels(floatCount);
    engine.readBuffer(screenshotBuffer, floatPixels.data(), byteCount);

    std::vector<uint8_t> pixels(static_cast<size_t>(screenshotWidth) * screenshotHeight * 4);
    auto toByte = [](float v) -> uint8_t {
        v = std::clamp(v, 0.0f, 1.0f);
        v = std::pow(v, 1.0f / 2.2f);
        return static_cast<uint8_t>(v * 255.0f + 0.5f);
    };
    for (size_t i = 0; i < floatCount; i += 4) {
        pixels[i + 0] = toByte(floatPixels[i + 0]);
        pixels[i + 1] = toByte(floatPixels[i + 1]);
        pixels[i + 2] = toByte(floatPixels[i + 2]);
        pixels[i + 3] = 255;
    }

    if (stbi_write_png(path.c_str(), static_cast<int>(screenshotWidth), static_cast<int>(screenshotHeight), 4, pixels.data(), static_cast<int>(screenshotWidth) * 4) != 0) {
        notificationManager.pushMessage(NotificationType::Info, "Saved screenshot to " + path);
    } else {
        notificationManager.pushMessage(NotificationType::Error, "Failed to write screenshot");
    }
}
