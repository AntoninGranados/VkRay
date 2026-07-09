#include "application.hpp"

#include <chrono>
#include <format>

#include "VkSmol/graph/render_graph_builder.hpp"
#include "app/log.hpp"
#include "core/core_renderer.hpp"
#include "core/export_service.hpp"
#include "imgui/imgui.h"
#include "FontAwesome/IconsFontAwesome7.h"

#include "VkSmol/render/shader.hpp"

#include "version.hpp"
#include "utils/progress.hpp"

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
        io.Fonts->AddFontFromFileTTF("assets/fonts/fa-solid-900.otf", 12.0f, &iconConfig, iconRanges);
    }

    ctx.reloadShaders = [this]() {
        coreRenderer.buildPipelines(engine);
        restartRender = true;
    };
    ctx.startRender = [this]() {
        if (ctx.renderMode == RenderMode::Preview)
            clearRenderingData(RenderMode::RenderSingle);
    };
    ctx.startRenderAnim = [this]() {
        if (ctx.renderMode == RenderMode::Preview) {
            clearRenderingData(RenderMode::RenderAnimation);
            animation.reset(0);
        }
    };
    ctx.getSampleCount = [this]() { return coreRenderer.getSampleCount(); };

    initParameters();
    initScene();
    RenderGraphBuilder builder;
    CoreResources resources = coreRenderer.initGraph(engine, builder);
    if (!platform.isHeadless())
        editorRenderer.initGraph(engine, builder, resources);
    engine.setGraph(builder);
    engine.initGraph();
    scene.setGpuBufferHandles(resources.sceneHandles);
}


Application::~Application() {
    engine.waitIdle();

    coreRenderer.destroy(engine);
    engine.destroyGraph();

    scene.destroy();
    engine.terminate();
}

void Application::run() {
    auto startTime = std::chrono::high_resolution_clock::now();

    while(!engine.shouldTerminate()) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        startTime = currentTime;

        scene.runPreUpdate(ctx);

        if (!animation.isPaused()) animation.step(deltaTime);

        onFrameStart(deltaTime);

        auto frameContext = engine.beginFrame();
        if (!frameContext) {
            engine.advanceFrame();
            scene.runPostUpdate(ctx);
            continue;
        }

        if (frameContext->swapchainGeneration != lastSwapchainGeneration) {
            lastSwapchainGeneration = frameContext->swapchainGeneration;
            VkExtent2D extent = engine.getExtent();
            coreRenderer.resize(engine, extent.width, extent.height);
            restartRender = true;
        }

        bool shouldSave = false;
        std::filesystem::path savePath;
        bool toVideo = false;

        if (ctx.renderMode != RenderMode::Preview && !(*ctx.restartRender)) {
            if (coreRenderer.isRenderFinished()) {
                shouldSave = true;

                if (ctx.renderMode == RenderMode::RenderAnimation) {
                    savePath = ExportService::buildAnimationFramePath(ctx.animation->getFrame());

                    ctx.animation->stepFixed();
                    if (ctx.animation->getFrame() == 0) {
                        toVideo = true;
                        ui->restoreToggledState();
                        ctx.renderMode = RenderMode::Preview;
                    }
                } else {
                    savePath = ctx.outputPath;
                    ui->restoreToggledState();
                    ctx.renderMode = RenderMode::Preview;
                }

                *ctx.restartRender = true;
            }
        }
        
        scene.runOnRender(ctx, *frameContext);

        coreRenderer.render(engine, *frameContext);
        editorRenderer.render(engine, *frameContext, [&](CommandBuffer& cmd) {
            ui->draw(cmd, ctx);
        });

        if (shouldSave) {
            coreRenderer.saveCapture(engine, savePath);
            if (toVideo) ExportService::convertFramesToVideo(ctx.outputPath);
        }

        engine.present();
        engine.advanceFrame();
        scene.runPostUpdate(ctx);
    }
}

void Application::runJobs(JobQueue& queue) {
    const int totalJobs = static_cast<int>(queue.entries().size());
    int jobIndex = 0;

    while (Job* job = queue.nextPending()) {
        jobIndex++;

        parameters.resetAll();
        for (const auto& paramOverride : job->parameterOverrides) {
            std::visit([&](auto&& v) {
                using T = std::decay_t<decltype(v)>;
                if constexpr      (std::is_same_v<T, bool>)        parameters.setBool(paramOverride.key, v);
                else if constexpr (std::is_same_v<T, int>)         parameters.setInt(paramOverride.key, v);
                else if constexpr (std::is_same_v<T, float>)       parameters.setFloat(paramOverride.key, v);
                else if constexpr (std::is_same_v<T, std::string>) parameters.setEnumByName(paramOverride.key, v);
            }, paramOverride.value);
        }

        const uint32_t totalSamples = job->samples;
        ProgressBar bar(
            "[" + std::to_string(jobIndex) + "/" + std::to_string(totalJobs) + "]",
            totalSamples,
            "spp"
        );

        const VkExtent2D extent = engine.getExtent();
        if (job->width != extent.width || job->height != extent.height)
            coreRenderer.resize(engine, job->width, job->height);

        LightMode lightMode = LightMode::Day;
        if (!SceneSerializer::load(scene, lightMode, job->scene.string(), job->seed)) {
            queue.fail();
            continue;
        }
        Log::success("Application", std::format("[{}/{}] Loaded: {}", jobIndex, totalJobs, job->scene.string()));
        ctx.camera = &scene.getCamera();
        parameters.setEnum<LightMode>("scene/light_mode", lightMode);

        engine.waitIdle();
        coreRenderer.reset();

        for (uint32_t i = 0; i < totalSamples; i++) {
            auto frameContext = engine.beginFrame();
            if (!frameContext) {
                engine.advanceFrame();
                scene.runPostUpdate(ctx);
                continue;
            }
            
            fillUBOs();
            coreRenderer.render(engine, *frameContext);
            queue.setProgress(static_cast<float>(i + 1) / static_cast<float>(totalSamples));
            bar.step();
        }
        bar.close();

        coreRenderer.saveCapture(engine, job->output);

        queue.complete();
    }
}

// Private
void Application::initParameters() {
    parameters.setLabel("pathtracer", "Pathtracer");
    parameters.addBool("pathtracer/denoising", "Denoising", false, false);

    parameters.addEnum<DebugView>(
        "pathtracer/debug_view",
        "Debug View",
        DebugView::None,
        { "None", "Position W", "Position", "Normal W", "Normal", "Albedo", "Roughness", "Mat Type", "Bounces", "Hit Checks", "Variance", "Selection Mask", "Sky Mask" },
        true
    );

    parameters.setLabel("pathtracer/sampling", "Sampling");
    parameters.addInt("pathtracer/sampling/max_bounces", "Max Bounces", 8, 1, 20, 1, false);
    parameters.addInt("pathtracer/sampling/render_samples", "Render Samples", 2048, 1, 4096, 1, false);
    parameters.addBool("pathtracer/sampling/importance_sampling", "Importance Sampling", true, false);
    // TODO: the clip threshold should be a parameter
    parameters.addBool("pathtracer/sampling/clip_accumulation", "Clip Accumulation", false, true);
    parameters.addBool("pathtracer/sampling/variance_sampling", "Variance Sampling", true, false);
    parameters.addInt("pathtracer/sampling/variance_warmup", "Variance Warmup Samples", 64, 0, 2048, 1, false);

    parameters.setLabel("pathtracer/aov", "Arbitrary Output Variables");
    parameters.addBool("pathtracer/aov/position_w", "Position W", false, false);
    parameters.addBool("pathtracer/aov/position",   "Position",   false, false);
    parameters.addBool("pathtracer/aov/normal_w",   "Normal W",   false, false);
    parameters.addBool("pathtracer/aov/normal",     "Normal",     false, false);
    parameters.addBool("pathtracer/aov/albedo",     "Albedo",     false, false);
    parameters.addBool("pathtracer/aov/roughness",  "Roughness",  false, false);
    parameters.addBool("pathtracer/aov/mat_type",   "Mat Type",   false, false);
    parameters.addBool("pathtracer/aov/sky_mask",   "Sky Mask",   false, false);

    parameters.setLabel("scene", "Scene");
    parameters.addEnum<LightMode>(
        "scene/light_mode",
        "Light Mode",
        LightMode::Day,
        { "Day", "Sunset", "Night", "Empty" },
        true
    );

    coreRenderer.bindParameters(parameters);

    parameters.bindEnum(
        "pathtracer/debug_view",
        [&](int v) {
            coreRenderer.setDebugView(v);
            editorRenderer.setDebugView(v);
        }
    );

    parameters.saveDocumentation();
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
    if (inputHandler) {
        inputHandler->pollEvents(ctx);
        inputHandler->handle(ctx, dt);
    }

    if (restartRender) {
        coreRenderer.reset();
        restartRender = false;
    }

    fillUBOs();
}

void Application::clearRenderingData(RenderMode newRenderMode) {
    scene.clearSelection();
    if (ui) {
        ui->saveToggledState();
        ui->setToggle(false);
    }
    ctx.renderMode = newRenderMode;
    coreRenderer.setTargetSampleCount(parameters.getInt("pathtracer/sampling/render_samples"));
    restartRender = true;
}


void Application::fillUBOs() {
    coreRenderer.setCamera(*ctx.camera);

    if (!platform.isHeadless())
        editorRenderer.setPreviewBorder(scene.isPreviewingCamera());
}
