#include "application.hpp"

#include <chrono>

#include "imgui/imgui.h"
#include "IconsFontAwesome7.h"

#include "scene/scene_preset.hpp"
#include "scene/object/object.hpp"

// Public
Application::Application() {
    inputHandler.initCallbacks(ctx);

    engine.init("VkRay", VK_MAKE_API_VERSION(0, 1, 0, 0));

    initParameters();
    initScene();
    renderer.init(ctx);

    {
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->AddFontDefault();
        ImFontConfig iconConfig;
        iconConfig.MergeMode = true;
        iconConfig.PixelSnapH = true;
        iconConfig.GlyphOffset = ImVec2(0.0f, 1.0f);
        static const ImWchar iconRanges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
        io.Fonts->AddFontFromFileTTF("res/fonts/fa-solid-900.otf", 12.0f, &iconConfig, iconRanges);
    }
}


Application::~Application() {
    engine.waitIdle();

    renderer.destroy(ctx);
    
    scene.destroy();
    engine.terminate();
}

void Application::run() {
    auto startTime = std::chrono::high_resolution_clock::now();

    while(!engine.shouldTerminate() && !shouldClose) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        startTime = currentTime;

        scene.runPreUpdate(ctx);

        if (!animation.isPaused()) animation.step(deltaTime);

        onFrameStart(deltaTime);
        renderer.render(ctx);

        scene.runPostUpdate(ctx);
    }
}

// Private
void Application::initParameters() {
    parameters.addInt("maxBounces", "Max Bounces", 8, 1, 20, 1, false, "Pathtracer");
    parameters.addInt("previewSamples", "Preview Samples", 1, 1, 10, 1, false, "Pathtracer");
    parameters.addInt("renderSamples", "Render Samples", 2048, 1, 4096, 1, false, "Pathtracer");
    parameters.addFloat("movingResolution", "Moving Resolution", 8.0f, 1.0f, 50.0f, 1.0f, false, "Pathtracer");
    parameters.addFloat("previewResolution", "Preview Resolution", 4.0f, 1.0f, 50.0f, 1.0f, true, "Pathtracer");
    parameters.addFloat("renderResolution", "Render Resolution", 1.0f, 1.0f, 50.0f, 1.0f, false, "Pathtracer");
    parameters.addBool("importanceSampling", "Importance Sampling", true, false, "Pathtracer");
    parameters.addBool("varianceSampling", "Variance Sampling", true, false, "Pathtracer");
    parameters.addBool("denoising", "Denoising", false, false, "Pathtracer");
    parameters.addInt("varianceWarmup", "Variance Warmup Samples", 64, 0, 2048, 1, false, "Pathtracer");
    parameters.addEnum(
        "debugView",
        "Debug View",
        static_cast<int>(DebugView::None),
        { "None", "Bounces", "Normal", "Position", "Diffuse", "Selection Mask", "Variance", "Hit Checks" },
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
    
    LightMode mode;
    scenePresetInitMethod[ScenePreset::Empty](*ctx.scene, mode);
    
    parameters.setEnum<LightMode>("lightMode", mode);
    ctx.camera = &scene.getCamera();
}

void Application::onFrameStart(float dt) {
    renderState.prevResolution = renderState.resolution;

    inputHandler.pollEvents();
    inputHandler.handle(ctx, dt);

    handleCommands();

    if (restartRender) {
        frameCount = 1;
        renderState.sampleCount = 0;
        restartRender = false;
        if (renderState.renderMode == RenderMode::Preview) renderState.resolution = parameters.getFloat("movingResolution");
    }

    fillUBOs();

    frameCount++;
    renderState.sampleCount += static_cast<uint64_t>(parameters.getInt("previewSamples"));
}

void Application::clearReaderingData(RenderMode newRenderMode) {
    scene.clearSelection();
    ui.saveToggledState();
    ui.setToggle(false);
    renderState.renderMode = newRenderMode;
    renderState.pendingExit = false;
    renderState.samplesPerSecEMA = 0.0;
    renderState.samplesPerSecInitialized = false;
    renderState.samplesPerSecAccumTime = 0.0;
    renderState.samplesPerSecAccumSamples = 0.0;
    restartRender = true;
}

void Application::handleCommands() {
    if (notifications.isCommandRequested(Command::Exit)) {
        shouldClose = true;
    } if (notifications.isCommandRequested(Command::Render)) {
        if (renderState.renderMode == RenderMode::Preview) {
            clearReaderingData(RenderMode::RenderSingle);
        }
    } if (notifications.isCommandRequested(Command::RenderAnim)) {
        if (renderState.renderMode == RenderMode::Preview) {
            clearReaderingData(RenderMode::RenderAnimation);
            animation.reset(0);
        }
    } if (notifications.isCommandRequested(Command::Reload)) {
        renderer.buildPipelines(ctx);
        restartRender = true;
    }
}

void Application::fillUBOs() {
    auto& pathtracer = *ctx.pathtracerUBO;
    auto& screen = *ctx.screenUBO;

    // Raytracing UBO
    pathtracer.cameraPos = ctx.camera->getPosition();
    pathtracer.cameraDir = ctx.camera->getDirection();
    pathtracer.tanHFov = ctx.camera->getTanHFov();
    pathtracer.aperture = ctx.camera->getAperture();
    pathtracer.focusDepth = ctx.camera->getFocusDepth();

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
    screen.previewBorderEnabled = scene.isPreviewingCamera() ? 1 : 0;
    screen.denoisingEnabled = static_cast<int>(parameters.getBool("denoising"));
}
