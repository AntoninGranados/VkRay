#include "application.hpp"

#include <string_view>

#include "VkSmol/graph/render_graph_builder.hpp"
#include "VkSmol/platform/glfw_platform.hpp"
#include "VkSmol/platform/headless_platform.hpp"
#include "VkSmol/render/shader.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

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
    platform = std::make_unique<GLFWPlatform>(std::format("VkRay [{}]", VK_RAY_VERSION_STRING), 1280, 720);
    Core::init(*platform, VK_RAY_VERSION);

    Editor::init();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;

    float xscale, yscale;
    glfwGetWindowContentScale(static_cast<GLFWwindow*>(platform->getNativeWindowHandle()), &xscale, &yscale);

    io.Fonts->AddFontFromFileTTF("assets/fonts/FiraCode-Regular.ttf", 14.0f * xscale);
    ImFontConfig iconConfig;
    iconConfig.MergeMode  = true;
    iconConfig.PixelSnapH = true;
    iconConfig.GlyphOffset = ImVec2(0.0f, 1.0f);
    static const ImWchar iconRanges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    io.Fonts->AddFontFromFileTTF("assets/fonts/fa-solid-900.otf", 14.0f * xscale, &iconConfig, iconRanges);
    io.FontGlobalScale = 1.0f / xscale;

    initScene();
    buildRenderGraph(false);
    runFn = Editor::run;
}

void Application::initOfflineMode(const std::string& jobFile) {
    JobQueue queue = JobQueue::fromFile(jobFile);
    if (queue.isEmpty()) return;

    platform = std::make_unique<HeadlessPlatform>(1080, 1080);  // This size is arbitrary as the buffers will be resized with the first jobs parameters
    Core::init(*platform, VK_RAY_VERSION);

    initScene();
    buildRenderGraph(true);
    // TODO: move job queue into Core; that removes the lifetime issue and this capture
    runFn = [q = std::move(queue)]() mutable { Offline::run(q); };
}

void Application::buildRenderGraph(bool offline) {
    RenderGraphBuilder builder;
    RenderResources resources = Core::getCoreRenderer().initGraph(builder);
    if (!offline) Editor::getEditorRenderer().initGraph(builder, resources);

    Core::getEngine().setGraph(builder);
    Core::getEngine().initGraph();

    if (!offline) Editor::getEditorRenderer().registerImGuiTextures();
    Core::getScene().setGpuBufferHandles(resources.sceneHandles);
}

void Application::initScene(const std::string& sceneFile) {
    Core::getScene().init();

    ecs::ComponentUiRegistry::init();

    LightMode mode = LightMode::Day;
    SceneSerializer::load(Core::getScene(), mode, sceneFile);

    Core::getParameters().set("scene/light_mode", mode);
}
