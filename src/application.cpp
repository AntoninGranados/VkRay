#include "application.hpp"

#include <chrono>
#include <format>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VkSmol/graph/render_graph_builder.hpp"
#include "VkSmol/render/shader.hpp"
#include "imgui/imgui.h"
#include "FontAwesome/IconsFontAwesome7.h"

#include "utils/log.hpp"
#include "utils/progress.hpp"
#include "version.hpp"
#include "core/export_service.hpp"
#include "core/scene/scene_serializer.hpp"
#include "editor/ecs/component_ui_registry.hpp"
#include "editor/editor.hpp"

// Public
Application::Application(Platform& p) : platform(p) {
    Shader::setSpvOutputDir(BUILD_DIR);
    engine.init("VkRay", VK_RAY_VERSION, platform);

    Core::init(engine, platform, parameters);

    if (!platform.isHeadless()) {
        Editor::init();

        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.Fonts->AddFontDefault();
        ImFontConfig iconConfig;
        iconConfig.MergeMode = true;
        iconConfig.PixelSnapH = true;
        iconConfig.GlyphOffset = ImVec2(0.0f, 1.0f);
        static const ImWchar iconRanges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
        io.Fonts->AddFontFromFileTTF("assets/fonts/fa-solid-900.otf", 12.0f, &iconConfig, iconRanges);
    }

    initParameters();
    initScene();
    RenderGraphBuilder builder;
    CoreResources resources = Core::getCoreRenderer().initGraph(builder);
    if (!platform.isHeadless())
        Editor::getEditorRenderer().initGraph(builder, resources);
    engine.setGraph(builder);
    engine.initGraph();
    if (!platform.isHeadless())
        Editor::getEditorRenderer().registerImGuiTextures();
    Core::getScene().setGpuBufferHandles(resources.sceneHandles);
}


Application::~Application() {
    engine.waitIdle();

    Core::getCoreRenderer().destroy();
    engine.destroyGraph();

    Core::getScene().destroy();
    engine.terminate();
}

void Application::run() {
    auto startTime = std::chrono::high_resolution_clock::now();

    while(!engine.shouldTerminate()) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        startTime = currentTime;

        Core::getScene().runPreUpdate();

        if (!Core::getAnimation().isPaused()) Core::getAnimation().step(deltaTime);

        onFrameStart(deltaTime);

        auto frameContext = engine.beginFrame();
        if (!frameContext) {
            engine.advanceFrame();
            Core::getScene().runPostUpdate();
            continue;
        }

        if (frameContext->swapchainGeneration != lastSwapchainGeneration) {
            lastSwapchainGeneration = frameContext->swapchainGeneration;
            if (platform.isHeadless()) {
                VkExtent2D extent = engine.getExtent();
                Core::getCoreRenderer().resize(extent.width, extent.height);
                Core::restartAccumulation();
            }
        }

        bool shouldSave = false;
        std::filesystem::path savePath;
        bool toVideo = false;

        if (Core::getRenderMode() != RenderMode::Preview && !Core::isAccumulationRestartPending()) {
            if (Core::getCoreRenderer().isRenderFinished()) {
                shouldSave = true;

                if (Core::getRenderMode() == RenderMode::RenderAnimation) {
                    savePath = ExportService::buildAnimationFramePath(Core::getAnimation().getFrame());

                    Core::getAnimation().stepFixed();
                    if (Core::getAnimation().getFrame() == 0) {
                        toVideo = true;
                        Editor::getUi().restoreToggledState();
                        Core::setRenderMode(RenderMode::Preview);
                    }
                } else {
                    savePath = Core::getOutputPath();
                    Editor::getUi().restoreToggledState();
                    Core::setRenderMode(RenderMode::Preview);
                }

                Core::requestAccumulationRestart();
            }
        }

        Core::getScene().runOnRender(*frameContext);

        if (!platform.isHeadless()) {
            const SceneSelection& sel = Editor::getUi().getSelection();
            int flatIdx = -1;
            if (sel.entity >= 0) {
                const ecs::Entity e = Core::getScene().getEntities()[static_cast<size_t>(sel.entity)];
                const ScenePackingMaps& maps = Core::getScene().getPackingMaps();
                int i = 0;
                auto check = [&](const std::unordered_map<ecs::Entity, int>& m) {
                    for (const auto& [ent, _] : m) {
                        if (ent == e) { flatIdx = i; return; }
                        i++;
                    }
                };
                check(maps.sphereId);
                if (flatIdx < 0) check(maps.planeId);
                if (flatIdx < 0) check(maps.boxId);
                if (flatIdx < 0) check(maps.quadId);
                if (flatIdx < 0) check(maps.meshId);
            }
            Core::getCoreRenderer().setSelectedObjectId(flatIdx);
        }

        Core::getCoreRenderer().render(*frameContext);
        Editor::getEditorRenderer().render(*frameContext);

        if (shouldSave) {
            Core::getCoreRenderer().saveCapture(savePath);
            if (toVideo) ExportService::convertFramesToVideo(Core::getOutputPath());
        }

        engine.present();
        engine.advanceFrame();
        Core::getScene().runPostUpdate();
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
            Core::getCoreRenderer().resize(job->width, job->height);

        LightMode lightMode = LightMode::Day;
        if (!SceneSerializer::load(Core::getScene(), lightMode, job->scene.string(), job->seed)) {
            queue.fail();
            continue;
        }
        Log::success("Application", std::format("[{}/{}] Loaded: {}", jobIndex, totalJobs, job->scene.string()));
        Core::setRenderMode(RenderMode::RenderSingle);
        parameters.setEnum<LightMode>("scene/light_mode", lightMode);

        engine.waitIdle();
        Core::restartAccumulation();

        for (uint32_t i = 0; i < totalSamples; i++) {
            auto frameContext = engine.beginFrame();
            if (!frameContext) {
                engine.advanceFrame();
                continue;
            }

            Core::getScene().runPreUpdate();
            Core::getScene().runOnRender(*frameContext);
            Core::getCoreRenderer().render(*frameContext);
            queue.setProgress(static_cast<float>(i + 1) / static_cast<float>(totalSamples));
            bar.step();
        }
        bar.close();

        Core::getCoreRenderer().saveCapture(job->output);
        Core::setRenderMode(RenderMode::Preview);

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

    parameters.addBool("pathtracer/sampling/clip_accumulation", "Clip Accumulation", false, true);
    // TODO: rename `clip_value` to `clip_threshold`
    parameters.addFloat("pathtracer/sampling/clip_value", "Clip Value", 50.0, 0.0, 1000.0, 0.01, true);

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

    Core::getCoreRenderer().bindParameters();

    parameters.saveDocumentation();
}

void Application::initScene(const std::string& sceneFile) {
    Core::getScene().init();

    auto& uiReg = ecs::ComponentUiRegistry::get();
    uiReg.setMaterials(&Core::getScene().getMaterials());
    uiReg.setMeshAssets(&Core::getScene().getMeshAssets());
    ecs::ComponentUiRegistry::init();

    LightMode mode = LightMode::Day;
    SceneSerializer::load(Core::getScene(), mode, sceneFile);

    parameters.setEnum<LightMode>("scene/light_mode", mode);
}

void Application::onFrameStart(float dt) {
    if (!platform.isHeadless()) {
        Editor::getInputHandler().pollEvents();
        Editor::getInputHandler().handle(dt);

        ImVec2 vpSize = Editor::getUi().getViewportSize();
        float xscale = 1.0f, yscale = 1.0f;
        glfwGetWindowContentScale(
            static_cast<GLFWwindow*>(platform.getNativeWindowHandle()),
            &xscale, &yscale);
        VkExtent2D vpExtent = {
            static_cast<uint32_t>(vpSize.x * xscale),
            static_cast<uint32_t>(vpSize.y * yscale)
        };
        VkExtent2D current = Core::getCoreRenderer().getRenderExtent();
        if (vpExtent.width > 0 && vpExtent.height > 0 &&
            (vpExtent.width != current.width || vpExtent.height != current.height)) {
            Core::getCoreRenderer().resize(vpExtent.width, vpExtent.height);
            Editor::getEditorRenderer().resize(vpExtent.width, vpExtent.height);
            Core::restartAccumulation();
        }
    }

    Core::consumeAccumulationRestart();
}


