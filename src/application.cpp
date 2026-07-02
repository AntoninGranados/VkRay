#include "application.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>

#include "imgui/imgui.h"
#include "FontAwesome/IconsFontAwesome7.h"

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

void Application::runHeadless(const std::filesystem::path& sceneFile, uint32_t targetSamples, const std::filesystem::path& outputPath) {
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
    auto& r = pathtracerUBO.render;
    auto& d = displayUBO;

    parameters.setLabel("pathtracer", "Pathtracer");
    parameters.addBool("pathtracer/denoising", "Denoising", false, false);
    parameters.bind("pathtracer/denoising", [&]() { d.denoisingEnabled = static_cast<int>(parameters.getBool("pathtracer/denoising")); });

    parameters.addEnum<DebugView>(
        "pathtracer/debug_view",
        "Debug View",
        DebugView::None,
        { "None", "Bounces", "Normal", "Position", "Diffuse", "Selection Mask", "Variance", "Hit Checks" },
        true
    );
    parameters.bind("pathtracer/debug_view", [&]() { r.debugView = d.debugView = static_cast<int>(parameters.getEnum<DebugView>("pathtracer/debug_view")); });

    parameters.setLabel("pathtracer/sampling", "Sampling");
    parameters.addInt("pathtracer/sampling/max_bounces", "Max Bounces", 8, 1, 20, 1, false);
    parameters.bindInt("pathtracer/sampling/max_bounces", &r.maxBounces);

    parameters.addInt("pathtracer/sampling/preview_samples", "Preview Samples", 1, 1, 10, 1, false);
    parameters.bindInt("pathtracer/sampling/preview_samples", &r.samplesPerPixel);

    parameters.addInt("pathtracer/sampling/render_samples", "Render Samples", 2048, 1, 4096, 1, false);

    parameters.addBool("pathtracer/sampling/importance_sampling", "Importance Sampling", true, false);
    parameters.bind("pathtracer/sampling/importance_sampling", [&]() { r.importanceSampling = static_cast<int>(parameters.getBool("pathtracer/sampling/importance_sampling")); });

    parameters.addBool("pathtracer/sampling/clip_accumulation", "Clip Accumulation", false, true);
    parameters.bind("pathtracer/sampling/clip_accumulation", [&]() { r.clipAccumulation = static_cast<int>(parameters.getBool("pathtracer/sampling/clip_accumulation")); });

    parameters.addBool("pathtracer/sampling/variance_sampling", "Variance Sampling", true, false);
    parameters.bind("pathtracer/sampling/variance_sampling", [&]() { r.varianceSampling = static_cast<int>(parameters.getBool("pathtracer/sampling/variance_sampling")); });

    parameters.addInt("pathtracer/sampling/variance_warmup", "Variance Warmup Samples", 64, 0, 2048, 1, false);
    parameters.bindInt("pathtracer/sampling/variance_warmup", &r.varianceWarmupSamples);

    parameters.setLabel("pathtracer/resolution", "Resolution");
    parameters.addFloat("pathtracer/resolution/moving",  "Moving Resolution",  8.0f, 1.0f, 50.0f, 1.0f, false);
    parameters.addFloat("pathtracer/resolution/preview", "Preview Resolution", 4.0f, 1.0f, 50.0f, 1.0f, true);
    parameters.addFloat("pathtracer/resolution/render",  "Render Resolution",  1.0f, 1.0f, 50.0f, 1.0f, false);

    parameters.setLabel("pathtracer/aov", "Arbitrary Output Variables");
    parameters.addBool("pathtracer/aov/normal",         "Normal",            false, false);
    parameters.addBool("pathtracer/aov/normal_opaque",  "Normal (opaque)",   false, false);
    parameters.addBool("pathtracer/aov/albedo",         "Albedo",            false, false);
    parameters.addBool("pathtracer/aov/albedo_opaque",  "Albedo (opaque)",   false, false);
    parameters.addBool("pathtracer/aov/depth",          "Depth",             false, false);
    parameters.addBool("pathtracer/aov/depth_opaque",   "Depth (opaque)",    false, false);
    parameters.addBool("pathtracer/aov/sky_mask",       "Sky mask",          false, false);
    parameters.addBool("pathtracer/aov/sky_mask_opaque","Sky mask (opaque)", false, false);

    parameters.setLabel("scene", "Scene");
    parameters.addEnum<LightMode>(
        "scene/light_mode",
        "Light Mode",
        LightMode::Day,
        { "Day", "Sunset", "Night", "Empty" },
        true
    );
    parameters.bind("scene/light_mode", [&]() { r.lightMode = static_cast<int>(parameters.getEnum<LightMode>("scene/light_mode")); });
}

void Application::initScene(const std::string& sceneFile) {
    scene.setContext(ctx);
    scene.init();

    LightMode mode = LightMode::Day;
    SceneSerializer::load(scene, mode, sceneFile);

    parameters.setEnum<LightMode>("scene/light_mode", mode);
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
        if (renderState.renderMode == RenderMode::Preview) renderState.resolution = parameters.getFloat("pathtracer/resolution/moving");
    }

    fillUBOs();

    frameCount++;
    renderState.sampleCount += static_cast<uint64_t>(parameters.getInt("pathtracer/sampling/preview_samples"));
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
            if (renderer.promptOutputPath())
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
    auto& display    = *ctx.displayUBO;

    pathtracer.camera.pos       = ctx.camera->getPosition();
    pathtracer.camera.dir       = ctx.camera->getDirection();
    pathtracer.camera.tanHFov   = ctx.camera->getTanHFov();
    pathtracer.camera.aperture  = ctx.camera->getAperture();
    pathtracer.camera.focusDepth = ctx.camera->getFocusDepth();

    VkExtent2D extent = engine.getExtent();
    pathtracer.screen.size         = { (float)extent.width, (float)extent.height };
    pathtracer.screen.aspect       = pathtracer.screen.size.x / pathtracer.screen.size.y;
    pathtracer.screen.resolution   = renderState.resolution;
    pathtracer.screen.prevResolution = renderState.prevResolution;

    if (frameCount <= 1) lastTime = (float)platform.getTime();
    pathtracer.frame.count = frameCount;
    pathtracer.frame.time  = (float)platform.getTime() - lastTime;

    display.frameCount           = frameCount;
    display.resolution           = renderState.resolution;
    display.previewBorderEnabled = scene.isPreviewingCamera() ? 1 : 0;
}

void Application::fillHeadlessUBOs(int sampleIndex, uint32_t targetSamples, LightMode lightMode) {
    auto& pathtracer = *ctx.pathtracerUBO;
    auto& display    = *ctx.displayUBO;

    pathtracer.camera.pos        = ctx.camera->getPosition();
    pathtracer.camera.dir        = ctx.camera->getDirection();
    pathtracer.camera.tanHFov    = ctx.camera->getTanHFov();
    pathtracer.camera.aperture   = ctx.camera->getAperture();
    pathtracer.camera.focusDepth = ctx.camera->getFocusDepth();

    VkExtent2D extent = engine.getExtent();
    pathtracer.screen.size           = { (float)extent.width, (float)extent.height };
    pathtracer.screen.aspect         = pathtracer.screen.size.x / pathtracer.screen.size.y;
    pathtracer.screen.resolution     = 1.0f;
    pathtracer.screen.prevResolution = 1.0f;

    pathtracer.frame.count = sampleIndex;
    pathtracer.frame.time  = 0.0f;

    pathtracer.render.lightMode             = static_cast<int>(lightMode);
    pathtracer.render.maxBounces            = 16;
    pathtracer.render.samplesPerPixel       = 1;
    pathtracer.render.importanceSampling    = 1;
    pathtracer.render.varianceSampling      = 0;
    pathtracer.render.varianceWarmupSamples = static_cast<int>(targetSamples / 2);
    pathtracer.render.clipAccumulation      = 1;
    pathtracer.render.debugView             = 0;

    display.frameCount           = sampleIndex;
    display.resolution           = 1.0f;
    display.debugView            = 0;
    display.previewBorderEnabled = 0;
    display.denoisingEnabled     = 0;
}
