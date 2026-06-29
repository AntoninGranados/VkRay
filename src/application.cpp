#include "application.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>

#include "imgui/imgui.h"
#include "IconsFontAwesome7.h"

#include "scene/object/object.hpp"
#include "VkSmol/render/shader.hpp"

#include "version.hpp"

// Public
Application::Application(Platform& p) : platform(p) {
    Shader::setSpvOutputDir(BUILD_DIR);
    engine.init("VkRay", VK_RAY_VERSION, platform);

    if (!platform.isHeadless()) {
        ui.emplace();
        ctx.ui = &(*ui);

        inputHandler.emplace();
        inputHandler->initCallbacks(ctx);

        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->AddFontDefault();
        ImFontConfig iconConfig;
        iconConfig.MergeMode = true;
        iconConfig.PixelSnapH = true;
        iconConfig.GlyphOffset = ImVec2(0.0f, 1.0f);
        static const ImWchar iconRanges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
        io.Fonts->AddFontFromFileTTF("res/fonts/fa-solid-900.otf", 12.0f, &iconConfig, iconRanges);
    }

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

void Application::runHeadless(const std::string& sceneFile, uint32_t targetSamples, const std::string& outputPath) {
    constexpr int kBarWidth = 40;

    LightMode lightMode = LightMode::Day;
    SceneSerializer::load(scene, lightMode, sceneFile);
    ctx.camera = &scene.getCamera();

    engine.waitIdle();
    restartRender = true;

    const auto startTime = std::chrono::steady_clock::now();

    for (uint32_t i = 0; i < targetSamples; ++i) {
        fillHeadlessUBOs(static_cast<int>(i), targetSamples, lightMode);
        renderer.renderHeadless(ctx, i == targetSamples - 1);

        const float progress = static_cast<float>(i + 1) / static_cast<float>(targetSamples);
        const int filled = static_cast<int>(progress * kBarWidth);

        const double elapsedSec = std::chrono::duration<double>(std::chrono::steady_clock::now() - startTime).count();
        const double etaSec  = elapsedSec / progress - elapsedSec;
        const int etaTotal   = static_cast<int>(etaSec);
        const int etaHours   = etaTotal / 3600;
        const int etaMins    = (etaTotal % 3600) / 60;
        const int etaSeconds = etaTotal % 60;

        std::cout << "\r[";
        for (int j = 0; j < kBarWidth; ++j)
            std::cout << (j < filled ? '=' : ' ');
        std::cout << "] " << static_cast<int>(progress * 100.0f)
                  << "% (" << (i + 1) << "/" << targetSamples << " spp)"
                  << "  ETA " << etaHours << ":"
                  << std::setw(2) << std::setfill('0') << etaMins << ":"
                  << std::setw(2) << std::setfill('0') << etaSeconds << "  ";
        std::cout.flush();
    }
    std::cout << '\n';

    VkExtent2D ext = engine.getExtent();
    renderer.saveCapture(ctx, outputPath, ext.width, ext.height);
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
    parameters.addBool("clipAccumulation", "Clip Accumulation", false, true, "Pathtracer");

    parameters.addEnum(
        "lightMode",
        "Light Mode",
        static_cast<int>(LightMode::Day),
        { "Day", "Sunset", "Night", "Empty" },
        true,
        "Scene"
    );
}

void Application::initScene(const std::string& sceneFile) {
    scene.setContext(ctx);
    scene.init();

    LightMode mode = LightMode::Day;
    SceneSerializer::load(scene, mode, sceneFile);

    parameters.setEnum<LightMode>("lightMode", mode);
    ctx.camera = &scene.getCamera();
}

void Application::onFrameStart(float dt) {
    renderState.prevResolution = renderState.resolution;

    if (inputHandler) {
        inputHandler->pollEvents(ctx);
        inputHandler->handle(ctx, dt);
    }

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

void Application::clearRenderingData(RenderMode newRenderMode) {
    scene.clearSelection();
    if (ui) {
        ui->saveToggledState();
        ui->setToggle(false);
    }
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
            clearRenderingData(RenderMode::RenderSingle);
        }
    } if (notifications.isCommandRequested(Command::RenderAnim)) {
        if (renderState.renderMode == RenderMode::Preview) {
            clearRenderingData(RenderMode::RenderAnimation);
            animation.reset(0);
        }
    } if (notifications.isCommandRequested(Command::Reload)) {
        renderer.buildPipelines(ctx);
        restartRender = true;
    }
}

void Application::fillUBOs() {
    auto& pathtracer = *ctx.pathtracerUBO;
    auto& screen     = *ctx.screenUBO;

    pathtracer.cameraPos  = ctx.camera->getPosition();
    pathtracer.cameraDir  = ctx.camera->getDirection();
    pathtracer.tanHFov    = ctx.camera->getTanHFov();
    pathtracer.aperture   = ctx.camera->getAperture();
    pathtracer.focusDepth = ctx.camera->getFocusDepth();

    VkExtent2D extent = engine.getExtent();
    pathtracer.screenSize     = { (float)extent.width, (float)extent.height };
    pathtracer.aspect         = pathtracer.screenSize.x / pathtracer.screenSize.y;
    pathtracer.resolution     = renderState.resolution;
    pathtracer.prevResolution = renderState.prevResolution;

    if (frameCount <= 1) lastTime = (float)platform.getTime();
    pathtracer.frameCount = frameCount;
    pathtracer.time = (float)platform.getTime() - lastTime;

    pathtracer.lightMode      = static_cast<int>(parameters.getEnum<LightMode>("lightMode"));
    pathtracer.maxBounces     = parameters.getInt("maxBounces");
    pathtracer.samplesPerPixel = parameters.getInt("previewSamples");
    pathtracer.importanceSampling    = static_cast<int>(parameters.getBool("importanceSampling"));
    pathtracer.varianceSampling      = static_cast<int>(parameters.getBool("varianceSampling"));
    pathtracer.varianceWarmupSamples = parameters.getInt("varianceWarmup");
    pathtracer.debugView      = static_cast<int>(parameters.getEnum<DebugView>("debugView"));
    pathtracer.clipAccumulation = static_cast<int>(parameters.getBool("clipAccumulation"));

    screen.frameCount           = frameCount;
    screen.resolution           = renderState.resolution;
    screen.debugView            = static_cast<int>(parameters.getEnum<DebugView>("debugView"));
    screen.previewBorderEnabled = scene.isPreviewingCamera() ? 1 : 0;
    screen.denoisingEnabled     = static_cast<int>(parameters.getBool("denoising"));
}

void Application::fillHeadlessUBOs(int sampleIndex, uint32_t targetSamples, LightMode lightMode) {
    auto& pathtracer = *ctx.pathtracerUBO;
    auto& screen     = *ctx.screenUBO;

    pathtracer.cameraPos  = ctx.camera->getPosition();
    pathtracer.cameraDir  = ctx.camera->getDirection();
    pathtracer.tanHFov    = ctx.camera->getTanHFov();
    pathtracer.aperture   = ctx.camera->getAperture();
    pathtracer.focusDepth = ctx.camera->getFocusDepth();

    VkExtent2D extent = engine.getExtent();
    pathtracer.screenSize     = { (float)extent.width, (float)extent.height };
    pathtracer.aspect         = pathtracer.screenSize.x / pathtracer.screenSize.y;
    pathtracer.resolution     = 1.0f;
    pathtracer.prevResolution = 1.0f;

    pathtracer.frameCount            = sampleIndex;
    pathtracer.time                  = 0.0f;
    pathtracer.lightMode             = static_cast<int>(lightMode);
    pathtracer.maxBounces            = 16;
    pathtracer.samplesPerPixel       = 1;
    pathtracer.importanceSampling    = 1;
    pathtracer.varianceSampling      = 0;
    pathtracer.varianceWarmupSamples = static_cast<int>(targetSamples / 2);
    pathtracer.debugView             = 0;
    pathtracer.clipAccumulation      = 0;

    screen.frameCount           = sampleIndex;
    screen.resolution           = 1.0f;
    screen.debugView            = 0;
    screen.previewBorderEnabled = 0;
    screen.denoisingEnabled     = 0;
}
