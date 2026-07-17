#include "application.hpp"

#include <string_view>

#include "VkSmol/graph/render_graph_builder.hpp"
#include "VkSmol/platform/glfw_platform.hpp"
#include "VkSmol/platform/headless_platform.hpp"
#include "VkSmol/render/shader.hpp"

#include "FontAwesome/IconsFontAwesome7.h"
#include "imgui/imgui.h"

#include "version.hpp"

#include "core/core.hpp"
#include "core/scene/scene_serializer.hpp"

#include "editor/ecs/component_ui_registry.hpp"
#include "editor/editor.hpp"

#include "offline/job_queue.hpp"
#include "offline/offline.hpp"

Application::Application(int argc, char* argv[]) {
    Shader::setSpvOutputDir(BUILD_DIR);

    if (argc >= 2 && std::string_view(argv[1]) == "--reference") {
        if (argc >= 3) Core::setOutputPath(argv[2]);
        initOfflineMode("assets/jobs/reference.json");
    } else if (argc >= 3 && std::string_view(argv[1]) == "--job") {
        initOfflineMode(argv[2]);
    } else {
        initEditorMode();
    }
}

void Application::run() {
    if (runFn) runFn();
}

Application::~Application() {
    Core::terminate();
}

void Application::initEditorMode() {
    platform = std::make_unique<GLFWPlatform>("VkRay", 1280, 720);
    Core::init(*platform, VK_RAY_VERSION);

    Editor::init();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.Fonts->AddFontFromFileTTF("assets/fonts/FiraCode-Regular.ttf", 14.0f);
    ImFontConfig iconConfig;
    iconConfig.MergeMode  = true;
    iconConfig.PixelSnapH = true;
    iconConfig.GlyphOffset = ImVec2(0.0f, 1.0f);
    static const ImWchar iconRanges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    io.Fonts->AddFontFromFileTTF("assets/fonts/fa-solid-900.otf", 14.0f, &iconConfig, iconRanges);

    initParameters();
    initScene();
    buildRenderGraph(true);
    runFn = Editor::run;
}

void Application::initOfflineMode(const std::string& jobFile) {
    JobQueue queue = JobQueue::fromFile(jobFile);
    if (queue.isEmpty()) return;

    platform = std::make_unique<HeadlessPlatform>(
        queue.entries().front().width,
        queue.entries().front().height
    );
    Core::init(*platform, VK_RAY_VERSION);

    initParameters();
    initScene();
    buildRenderGraph(false);
    // TODO: move job queue into Core; that removes the lifetime issue and this capture
    runFn = [q = std::move(queue)]() mutable { Offline::run(q); };
}

void Application::buildRenderGraph(bool withEditor) {
    RenderGraphBuilder builder;
    CoreResources resources = Core::getCoreRenderer().initGraph(builder);
    if (withEditor)
        Editor::getEditorRenderer().initGraph(builder, resources);
    Core::getEngine().setGraph(builder);
    Core::getEngine().initGraph();
    if (withEditor)
        Editor::getEditorRenderer().registerImGuiTextures();
    Core::getScene().setGpuBufferHandles(resources.sceneHandles);
}

void Application::initParameters() {
    ParameterHandler& params = Core::getParameters();
    params.setLabel("pathtracer", "Pathtracer");
    params.addBool("pathtracer/denoising", "Denoising", false, false);

    params.addEnum<DebugView>(
        "pathtracer/debug_view",
        "Debug View",
        DebugView::None,
        { "None", "Position W", "Position", "Normal W", "Normal", "Albedo", "Roughness", "Mat Type", "Bounces", "Hit Checks", "Variance", "Selection Mask", "Sky Mask" },
        true
    );

    params.setLabel("pathtracer/sampling", "Sampling");
    params.addInt("pathtracer/sampling/max_bounces", "Max Bounces", 8, 1, 20, 1, false);
    params.addInt("pathtracer/sampling/render_samples", "Render Samples", 2048, 1, 4096, 1, false);
    params.addBool("pathtracer/sampling/importance_sampling", "Importance Sampling", true, false);

    params.addBool("pathtracer/sampling/clip_accumulation", "Clip Accumulation", false, true);
    // TODO: rename `clip_value` to `clip_threshold`
    params.addFloat("pathtracer/sampling/clip_value", "Clip Value", 50.0, 0.0, 1000.0, 0.01, true);

    params.addBool("pathtracer/sampling/variance_sampling", "Variance Sampling", true, false);
    params.addInt("pathtracer/sampling/variance_warmup", "Variance Warmup Samples", 64, 0, 2048, 1, false);

    params.setLabel("pathtracer/aov", "Arbitrary Output Variables");
    params.addBool("pathtracer/aov/position_w", "Position W", false, false);
    params.addBool("pathtracer/aov/position",   "Position",   false, false);
    params.addBool("pathtracer/aov/normal_w",   "Normal W",   false, false);
    params.addBool("pathtracer/aov/normal",     "Normal",     false, false);
    params.addBool("pathtracer/aov/albedo",     "Albedo",     false, false);
    params.addBool("pathtracer/aov/roughness",  "Roughness",  false, false);
    params.addBool("pathtracer/aov/mat_type",   "Mat Type",   false, false);
    params.addBool("pathtracer/aov/sky_mask",   "Sky Mask",   false, false);

    params.setLabel("scene", "Scene");
    params.addEnum<LightMode>(
        "scene/light_mode",
        "Light Mode",
        LightMode::Day,
        { "Day", "Sunset", "Night", "Empty" },
        true
    );

    Core::getCoreRenderer().bindParameters();
    params.saveDocumentation();
}

void Application::initScene(const std::string& sceneFile) {
    Core::getScene().init();

    auto& uiReg = ecs::ComponentUiRegistry::get();
    uiReg.setMaterials(&Core::getScene().getMaterials());
    uiReg.setMeshAssets(&Core::getScene().getMeshAssets());
    ecs::ComponentUiRegistry::init();

    LightMode mode = LightMode::Day;
    SceneSerializer::load(Core::getScene(), mode, sceneFile);

    Core::getParameters().setEnum<LightMode>("scene/light_mode", mode);
}
