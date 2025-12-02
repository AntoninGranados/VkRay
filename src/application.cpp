#include "application.hpp"

const std::vector<ScreenVertex> vertices = {
    { .position={ 1.0f, 1.0f} },
    { .position={ 1.0f,-1.0f} },
    { .position={-1.0f,-1.0f} },
    { .position={-1.0f, 1.0f} }
};

const std::vector<index_t> indices = {
    0, 1, 2, 2, 3, 0
};

Application::Application() {
    engine.init("VkRay", VK_MAKE_API_VERSION(0, 1, 0, 0));

    glfwSetWindowAttrib(engine.getWindow().get(), GLFW_RESIZABLE, GLFW_FALSE);
    glfwSetCursorPosCallback(
        engine.getWindow().get(),
        [](GLFWwindow* window, double x, double y) {
            ImGui_ImplGlfw_CursorPosCallback(window, x, y);
            auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
            if (app->camera.cursorPosCallback(window, x, y))
                app->frameCount = 1;
        }
    );
    glfwSetScrollCallback(
        engine.getWindow().get(), 
        [](GLFWwindow* window, double xoffset, double yoffset) {
            ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
            auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
            if (app->camera.scrollCallback(window, xoffset, yoffset))
                app->frameCount = 1;
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
    }

    {   // Image (image + view + sampler) creation
        VkExtent2D extent = engine.getExtent();
        for (size_t i = 0; i < 2; i++) {
            images[i] = engine.initImage(
                extent.width, extent.height,
                // Use float format to avoid quantizing every accumulation step
                VK_FORMAT_R32G32B32A32_SFLOAT,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            );
            imageViews[i] = engine.initImageView(images[i]);
            samplers[i] = engine.initSampler();
        }
    }
    
    setLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    setLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    setLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    engine.initDescriptorSetLayout(setLayout);
    
    screenSetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    screenSetLayout.addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
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

    initScene();

    {   // Descriptor sets creation
        std::vector<std::pair<ImageView, Sampler> > combinedImageSampler = {
            { imageViews[0], samplers[0] },
            { imageViews[1], samplers[1] }
        };
        
        bufferList_t storageBuffer = scene.getBufferList();
        for (size_t i = 0; i < 2; i++) {
            descriptorSets[i] = engine.initDescriptorSetList(
                setLayout,
                { &raytracingUniformBuffers, &storageBuffer, &combinedImageSampler[1-i] }
            );
            screenDescriptorSets[i] = engine.initDescriptorSetList(
                screenSetLayout,
                { &combinedImageSampler[i], &screenUniformBuffers }
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
    scene.destroy(engine);
    
    engine.destroyGraphicsPipeline(pipeline);
    engine.destroyGraphicsPipeline(screenPipeline);
    
    engine.terminate();
}


void Application::initScene() {
    scene.init(engine);

    scene.pushSphere({
        .center = { -2.0, 0.0, 0.0 },
        .radius = 1.0 - 0.01,
        .mat.type = dielectric,
        .mat.albedo = { 0.95, 0.8, 0.9 },
        .mat.refraction_index = 1.5,
    }, "Glass");

    // scene.pushSphere({
    //     .center = { -2.0, 0.0, 0.0 },
    //     .radius = (1.0 - 0.01) * 0.9,
    //     .mat.type = dielectric,
    //     .mat.albedo = { 0.9, 0.9, 0.9 },
    //     .mat.refraction_index = 1.0 / 1.5,
    // }, "Glass inner");

    scene.pushSphere({
        .center = { 0.0, 0.0, 0.0 },
        .radius = 1.0 - 0.01,
        .mat.albedo = { 0.2, 0.7, 0.2 },
        .mat.type = lambertian,
    }, "Diffuse");

    scene.pushSphere({
        .center = { 2.0, 0.0, 0.0 },
        .radius = 1.0 - 0.01,
        .mat.type = metal,
        .mat.albedo = { 0.8, 0.6, 0.2 },
        .mat.fuzz = 0.01,
    }, "Metal");

    scene.pushSphere({
        .center = { 0.0, 2.0, 6.0 },
        .radius = 3.0 - 0.01,
        .mat.type = emissive,
        .mat.albedo = { 1.0, 0.1, 0.04 },
        .mat.intensity = 10.0,
    }, "Light");
}


void Application::run() {
    auto startTime = std::chrono::high_resolution_clock::now();

    while(!engine.shouldTerminate()) {
        frame = (frame + 1) % 2;
        frameCount++;

        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        startTime = currentTime;

        CommandBuffer commandBuffer;
        
        engine.beginFrame();

        if (camera.processInput(engine.getWindow().get(), deltaTime)) {
            frameCount = 1;
        }
        if (camera.isLocked())
            glfwSetInputMode(engine.getWindow().get(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        else
            glfwSetInputMode(engine.getWindow().get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        if (glfwGetKey(engine.getWindow().get(), GLFW_KEY_H) == GLFW_PRESS) {
            rebuildPipeline();
            frameCount = 0;
        }
        if (glfwGetKey(engine.getWindow().get(), GLFW_KEY_R) == GLFW_PRESS) {
            frameCount = 0;
        }

        RaytracingUBO raytracingUBO;
        ScreenUBO screenUBO;
        fillUBOs(raytracingUBO, screenUBO);
        
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
                
                engine.fillBuffer(engine.getBuffer(raytracingUniformBuffers), &raytracingUBO);
                // engine.fillBuffer(engine.getBuffer(scene.getBufferList()), &ssbo);
                scene.fillBuffer(engine);
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
    }
}


// #include <future>
void Application::drawUI(CommandBuffer commandBuffer) {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    
    ImGui::NewFrame();

    ImGuizmo::SetOrthographic(false);
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
        ImGuiWindowFlags_NoDocking
    );
    {
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 windowSize = ImGui::GetWindowSize();
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(windowPos.x, windowPos.y, windowSize.x, windowSize.y);
        
        Sphere* sphere = scene.getSelectedSphere();
        if (sphere != nullptr) {
            glm::mat4 model = glm::translate(glm::mat4(1.0), sphere->center);
            model = glm::scale(model, glm::vec3(sphere->radius));

            if (ImGuizmo::Manipulate(
                glm::value_ptr(camera.getView()),
                glm::value_ptr(camera.getProjection(engine.getWindow().get())),
                ImGuizmo::OPERATION::SCALE_X | ImGuizmo::OPERATION::TRANSLATE,
                ImGuizmo::MODE::WORLD, 
                glm::value_ptr(model)
            )) {
                float translation[3], rotation[3], scale[3];
                ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(model), translation, rotation, scale);
                sphere->center = { translation[0], translation[1], translation[2] };
                sphere->radius = scale[0];

                frameCount = 0;
            }
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
    
    ImGui::SetNextWindowPos({0, 0});
    ImGui::Begin("FPS",
        nullptr,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration // | ImGuiWindowFlags_NoBackground
    );
    ImGui::Text("%.1f fps (%.3f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
    ImGui::End();

    ImGui::SetNextWindowBgAlpha(0.5f);
    ImGui::Begin("Information");
    {
        ImGui::Text("Camera Position:\n (%4.1f, %4.1f, %4.1f)", camera.getPosition().x, camera.getPosition().y, camera.getPosition().z);
        ImGui::Separator();
        ImGui::Text("Camera Direction:\n (%4.1f, %4.1f, %4.1f)", camera.getDirection().x, camera.getDirection().y, camera.getDirection().z);
        ImGui::Separator();
        ImGui::Text("Camera Fov:\n %4.1f°", camera.getFov());
        ImGui::Separator();

        const char *cycle[3] = { "Day", "Sunset", "Night" };
        ImGui::PushItemWidth(-FLT_MIN);
        if (ImGui::Combo("##TimeOfDay", &timeOfDay, cycle, IM_ARRAYSIZE(cycle))) frameCount = 0;
        ImGui::PopItemWidth();

        ImGui::Separator();
        
        ImGui::PushItemWidth(-FLT_MIN);
        if (ImGui::Button("Hot Reload Shader (H)", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            // std::async(&Application::rebuildPipeline, this);
            rebuildPipeline();
            frameCount = 0;
        }
        ImGui::PopItemWidth();
        
        ImGui::PushItemWidth(-FLT_MIN);
        if (ImGui::Button("Reset Accumulation (R)", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
            frameCount = 0;
        ImGui::PopItemWidth();
        ImGui::Separator();
        
        scene.drawUI(frameCount);
    }
    ImGui::End();
    
    ImGui::Render();
    
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer.get());
}

void Application::fillUBOs(RaytracingUBO &raytracingUBO, ScreenUBO &screenUBO) {
    // Raytracing UBO
    if (frameCount <= 1)
        lastTime = glfwGetTime();
    raytracingUBO.time = glfwGetTime() - lastTime;
    raytracingUBO.frameCount = frameCount;

    VkExtent2D extent = engine.getExtent();
    raytracingUBO.screenSize = { (float)extent.width, (float)extent.height };
    raytracingUBO.aspect = raytracingUBO.screenSize.x / raytracingUBO.screenSize.y;

    raytracingUBO.tanHFov = camera.getTanHFov();
    raytracingUBO.cameraPos = camera.getPosition();
    raytracingUBO.cameraDir = camera.getDirection();

    raytracingUBO.timeOfDay = timeOfDay;

    // Screen UBO
    screenUBO.sphereCount = scene.getSpheresCount();
    screenUBO.sphereId = scene.getSelectedSphereId() + 1;
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
        return;
    }
    
    std::string fragShaderPath = "./res/shader/raytracing.glsl";
    Shader fragShader;
    try {
        fragShader = engine.initShader(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderPath);
    } catch (...) {
        engine.destroyShader(vertShader);
        std::cerr << "[ERROR] Failed to compile shader [" << fragShaderPath << "]: pipeline not built" << std::endl;
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
}
