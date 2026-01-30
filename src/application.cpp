#include "application.hpp"

#include <algorithm>
#include <cmath>

const float PREVIEW_VIEWPORT_SCALE = 0.8f;

Application::Application() {
    engine.init("VkRay", VK_MAKE_API_VERSION(0, 1, 0, 0));

    glfwSetCursorPosCallback(
        engine.getWindow().get(),
        [](GLFWwindow* window, double x, double y) {
            ImGui_ImplGlfw_CursorPosCallback(window, x, y);
            auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
            const bool cameraLocked = app->camera.isLocked();
            if (cameraLocked && (ImGui::GetIO().WantCaptureMouse || app->ui.isMouseCaptured() || ImGuizmo::IsUsing()))
                return;
            app->restartRender |= app->camera.cursorPosCallback(window, x, y);
        }
    );

    glfwSetScrollCallback(
        engine.getWindow().get(), 
        [](GLFWwindow* window, double xoffset, double yoffset) {
            ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
            auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
            if (ImGui::GetIO().WantCaptureMouse || app->ui.isMouseCaptured()) return;
            app->restartRender |= app->camera.scrollCallback(window, xoffset, yoffset);
        }
    );

    initParameters();
    initScene();
    renderer.init(ctx);
}


Application::~Application() {
    engine.waitIdle();

    renderer.destroy(ctx);
    
    scene.destroy();
    engine.terminate();
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
        { "None", "Bounces", "Normal", "Selection Mask", "Variance", "Hit Checks" },
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

void Application::initScene() {
    scene.setContext(ctx);
    scene.init();

    scene.setPreviewCameraCallback([this](const CameraHandle &handle) {
        if (cameraHandle && scene.containsObject(cameraHandle))
            cameraHandle->setPreview(false);
        float dist = glm::length(camera.getTarget() - camera.getPosition());
        if (dist < 0.1f) dist = 1.0f;
        camera.setPosition(handle.getPosition());
        camera.setTarget(handle.getPosition() + handle.getDirection() * dist);
        float fovRad = glm::radians(handle.getFov());
        float previewFovRad = 2.0f * atanf(tanf(fovRad * 0.5f) / PREVIEW_VIEWPORT_SCALE);
        camera.setFov(glm::degrees(previewFovRad));
        camera.setAperture(handle.getAperture());
        camera.setFocusDepth(handle.getFocusDepth());
        cameraHandle = const_cast<CameraHandle*>(&handle);
        cameraHandle->setPreview(true);
        restartRender = true;
    });

    LightMode mode = scene.loadPreset(ScenePreset::Empty);
    parameters.setEnum<LightMode>("lightMode", mode);
}


void Application::run() {
    auto startTime = std::chrono::high_resolution_clock::now();

    while(!engine.shouldTerminate() && !shouldClose) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        startTime = currentTime;
        
        onFrameStart(deltaTime);
        renderer.render(ctx);
    }
}


void Application::onFrameStart(float dt) {
    scene.runSystems(ctx);

    renderState.prevResolution = renderState.resolution;

    syncCameraFromHandle();
    handleInput(dt);
    syncHandleFromCamera();

    if (notifications.isCommandRequested(Command::Exit)) {
        shouldClose = true;
    } if (notifications.isCommandRequested(Command::Render)) {
        if (!renderState.renderMode) {
            scene.clearSelection();
            ui.saveToggledState();
            ui.setToggle(false);
            CameraHandle* firstCamera = scene.getFirstCameraHandle();
            if (firstCamera) {
                float dist = glm::length(camera.getTarget() - camera.getPosition());
                if (dist < 0.1f) dist = 1.0f;
                camera.setPosition(firstCamera->getPosition());
                camera.setTarget(firstCamera->getPosition() + firstCamera->getDirection() * dist);
                camera.setFov(firstCamera->getFov());
                camera.setAperture(firstCamera->getAperture());
                camera.setFocusDepth(firstCamera->getFocusDepth());
            }
            renderState.renderMode = true;
            renderState.pendingExit = false;
            renderState.samplesPerSecEMA = 0.0;
            renderState.samplesPerSecInitialized = false;
            renderState.samplesPerSecAccumTime = 0.0;
            renderState.samplesPerSecAccumSamples = 0.0;
            restartRender = true;
        }
    } if (notifications.isCommandRequested(Command::Reload)) {
        renderer.buildPipeline(ctx);
        restartRender = true;
    }

    if (restartRender) {
        frameCount = 1;
        renderState.sampleCount = 0;
        restartRender = false;
        if (!renderState.renderMode) renderState.resolution = parameters.getFloat("movingResolution");
    }

    fillUBOs();

    frameCount++;
    renderState.sampleCount += static_cast<uint64_t>(parameters.getInt("previewSamples"));
}

void Application::syncHandleFromCamera() {
    if (!cameraHandle) return;
    if (!scene.containsObject(cameraHandle)) {
        cameraHandle = nullptr;
        return;
    }

    cameraHandle->setPosition(camera.getPosition());
    cameraHandle->setDirection(camera.getDirection());
    float fovRad = glm::radians(camera.getFov());
    float handleFovRad = 2.0f * atanf(tanf(fovRad * 0.5f) * PREVIEW_VIEWPORT_SCALE);
    cameraHandle->setFov(glm::max(1.0f, glm::degrees(handleFovRad)));
    cameraHandle->setAperture(camera.getAperture());
    cameraHandle->setFocusDepth(camera.getFocusDepth());
}

void Application::syncCameraFromHandle() {
    if (!cameraHandle) return;
    if (!scene.containsObject(cameraHandle)) {
        cameraHandle = nullptr;
        return;
    }

    float dist = glm::length(camera.getTarget() - camera.getPosition());
    if (dist < 0.1f) dist = 1.0f;
    camera.setPosition(cameraHandle->getPosition());
    camera.setTarget(cameraHandle->getPosition() + cameraHandle->getDirection() * dist);
    float fovRad = glm::radians(cameraHandle->getFov());
    float previewFovRad = 2.0f * atanf(tanf(fovRad * 0.5f) / PREVIEW_VIEWPORT_SCALE);
    camera.setFov(glm::degrees(previewFovRad));
    camera.setAperture(cameraHandle->getAperture());
    camera.setFocusDepth(cameraHandle->getFocusDepth());
}


void Application::handleInput(float dt) {
    glfwPollEvents();

    if (renderState.renderMode)
        handleInputRender(dt);
    else
        handleInputPreview(dt);
}

void Application::handleInputRender(float dt) {
    renderState.resolution = parameters.getFloat("renderResolution");

    double dtSafe = std::max(static_cast<double>(dt), 0.0);
    renderState.samplesPerSecAccumTime += dtSafe;
    renderState.samplesPerSecAccumSamples += static_cast<double>(parameters.getInt("previewSamples"));
    if (renderState.samplesPerSecAccumTime >= 1.0) {
        double instant = renderState.samplesPerSecAccumSamples / std::max(renderState.samplesPerSecAccumTime, 1e-6);
        double alpha = 1.0 - std::exp(-renderState.samplesPerSecAccumTime / 5.0);
        if (!renderState.samplesPerSecInitialized) {
            renderState.samplesPerSecEMA = instant;
            renderState.samplesPerSecInitialized = true;
        } else {
            renderState.samplesPerSecEMA += alpha * (instant - renderState.samplesPerSecEMA);
        }
        renderState.samplesPerSecAccumTime = 0.0;
        renderState.samplesPerSecAccumSamples = 0.0;
    }

    glfwSetInputMode(engine.getWindow().get(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    if (glfwGetKey(engine.getWindow().get(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        ui.restorToggledState();
        renderState.renderMode = false;
        renderState.pendingExit = false;
        renderState.samplesPerSecEMA = 0.0;
        renderState.samplesPerSecInitialized = false;
        renderState.samplesPerSecAccumTime = 0.0;
        renderState.samplesPerSecAccumSamples = 0.0;
    }
}

void Application::handleInputPreview(float dt) {
    renderState.resolution = parameters.getFloat("previewResolution");
    
    const bool blockMouseInput = ImGuizmo::IsUsing() || (camera.isLocked() && (ui.isMouseCaptured() || ImGui::GetIO().WantCaptureMouse));
    const bool blockKeyboardInput = ui.isKeyboardCaptured() || ImGui::GetIO().WantCaptureKeyboard;

    const bool middleDown = glfwGetMouseButton(engine.getWindow().get(), GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
    if (!blockMouseInput && middleDown && !ui.wasMiddleClickDown()) {
        double xpos, ypos;
        glfwGetCursorPos(engine.getWindow().get(), &xpos, &ypos);
        int width, height;
        glfwGetWindowSize(engine.getWindow().get(), &width, &height);
        float dist;
        glm::vec3 p;
        if (scene.raycast({ xpos, ypos }, { static_cast<float>(width), static_cast<float>(height) }, camera, dist, p, false, false)) {
            camera.setFocusDepth(dist);
            if (cameraHandle)
                cameraHandle->setFocusDepth(dist);
            restartRender = true;
        }
    }
    ui.setMiddleClickState(middleDown);

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
        if (!ui.isToggled()) ui.toggle();
        else scene.clearSelection();
    }
    
    if (!blockKeyboardInput) {
        restartRender |= camera.processInput(engine.getWindow().get(), dt);
    }
    
    if (camera.isLocked() || blockMouseInput)
        glfwSetInputMode(engine.getWindow().get(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    else
        glfwSetInputMode(engine.getWindow().get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    if (!blockKeyboardInput && glfwGetKey(engine.getWindow().get(), GLFW_KEY_R) == GLFW_PRESS)
    restartRender = true;

    if (cameraHandle != nullptr && glfwGetKey(engine.getWindow().get(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        if (cameraHandle) cameraHandle->setPreview(false);
        cameraHandle = nullptr;
        camera.setAperture(0.0f);
        restartRender = true;
    }
    
    if (scene.checkUpdate()) restartRender = true;
}


void Application::fillUBOs() {
    auto& pathtracer = *ctx.pathtracerUBO;
    auto& screen = *ctx.screenUBO;

    // Raytracing UBO
    pathtracer.cameraPos = camera.getPosition();
    pathtracer.cameraDir = camera.getDirection();
    pathtracer.tanHFov = camera.getTanHFov();
    pathtracer.aperture = camera.getAperture();
    pathtracer.focusDepth = camera.getFocusDepth();

    VkExtent2D extent = engine.getExtent();
    pathtracer.screenSize = { (float)extent.width, (float)extent.height };
    pathtracer.aspect = pathtracer.screenSize.x / pathtracer.screenSize.y;
    pathtracer.resolution = renderState.resolution;
    pathtracer.prevResolution = renderState.prevResolution;

    if (frameCount <= 1)
        lastTime = glfwGetTime();
    pathtracer.frameCount = frameCount;
    pathtracer.time = glfwGetTime() - lastTime;
    
    pathtracer.lightMode = static_cast<int>(parameters.getEnum<LightMode>("lightMode"));

    pathtracer.maxBounces = parameters.getInt("maxBounces");
    pathtracer.samplesPerPixel = parameters.getInt("previewSamples");
    pathtracer.importanceSampling = static_cast<int>(parameters.getBool("importanceSampling"));
    pathtracer.varianceSampling = static_cast<int>(parameters.getBool("varianceSampling"));
    pathtracer.varianceWarmupSamples = parameters.getInt("varianceWarmup");
    pathtracer.debugView = static_cast<int>(parameters.getEnum<DebugView>("debugView"));

    // Screen UBO
    screen.frameCount = frameCount;
    screen.resolution = renderState.resolution;
    screen.debugView = static_cast<int>(parameters.getEnum<DebugView>("debugView"));
    screen.previewBorderEnabled = cameraHandle ? 1 : 0;
}
