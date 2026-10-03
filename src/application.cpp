#include "application.hpp"

#include <filesystem>
#include <string>
#include <string_view>

#include "VkSmol/graph/render_graph_builder.hpp"
#include "VkSmol/platform/glfw_platform.hpp"
#include "VkSmol/platform/headless_platform.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "FontAwesome/IconsFontAwesome7.h"
#include "imgui/imgui.h"

#include "version.hpp"

#include "core/core.hpp"
#include "core/ecs/components/component_serializer.hpp"
#include "core/fields/parameter_serializer.hpp"
#include "core/scene/scene_serializer.hpp"
#include "core/shader_plugin/shader_plugin.hpp"
#include "core/shader_plugin/shader_source_map.hpp"

#include "editor/ecs/component_ui_registry.hpp"
#include "editor/editor.hpp"

#include "offline/job_queue.hpp"
#include "offline/offline.hpp"

#include "utils/log.hpp"
#include "utils/resources.hpp"

Application::Application(int argc, char* argv[]) {
    if (argc >= 3 && std::string_view(argv[1]) == "--job") {
        initOfflineMode(argv[2]);
    } else if (argc >= 3 && std::string_view(argv[1]) == "--generate-docs") {
        generateDocumentation(argv[2]);
    } else {
        initEditorMode();
    }
}

void Application::run() {
    if (runFn) runFn();
}

Application::~Application() {
    if (!platform) return;
    if (!platform->isHeadless()) Editor::terminate();
    Core::terminate();
}

void Application::generateDocumentation(const std::filesystem::path& directory) {
    std::filesystem::create_directories(directory);
    Core::loadParameters();
    // TODO: move that to a meta programm (compile time)
    ParameterSerializer::saveDocumentation(directory / "parameters.md");
    ComponentSerializer::saveDocumentation(directory / "components.md");
}

void Application::initEditorMode() {
    platform = std::make_unique<GLFWPlatform>(std::format("VkRay [{}]", VK_RAY_VERSION_STRING), 1280, 720);
    Core::init(*platform, VK_RAY_VERSION);

    Editor::init();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;

    float xscale, yscale;
    glfwGetWindowContentScale(static_cast<GLFWwindow*>(platform->getNativeWindowHandle()), &xscale, &yscale);

    const auto addFont = [&](const char* path, ImFontConfig config, const ImWchar* ranges = nullptr) {
        const std::string_view font = Resources::find(path).value();
        config.FontDataOwnedByAtlas = false;
        io.Fonts->AddFontFromMemoryTTF(const_cast<char*>(font.data()), static_cast<int>(font.size()), 14.0f * xscale,
                                       &config, ranges);
    };
    addFont("builtin:/fonts/FiraCode-Regular.ttf", ImFontConfig());

    ImFontConfig iconConfig;
    iconConfig.MergeMode = true;
    iconConfig.PixelSnapH = true;
    iconConfig.GlyphOffset = ImVec2(0.0f, 1.0f);
    static const ImWchar iconRanges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
    addFont("builtin:/fonts/fa-solid-900.otf", iconConfig, iconRanges);
    io.FontGlobalScale = 1.0f / xscale;

    initScene();
    Core::installGraphBuilder([this] { buildRenderGraph(false); });
    runFn = Editor::run;
}

void Application::initOfflineMode(const std::string& jobFile) {
    JobQueue queue;
    try {
        queue = JobQueue::fromFile(jobFile);
    } catch (const std::exception& e) {
        Log::error("Application", std::format("Failed to load job queue `{}`: {}", jobFile, e.what()));
        return;
    }
    if (queue.isEmpty()) return;

    platform = std::make_unique<HeadlessPlatform>(
        1080, 1080); // This size is arbitrary as the buffers will be resized with the first jobs parameters
    Core::init(*platform, VK_RAY_VERSION);

    initScene();
    Core::installGraphBuilder([this] { buildRenderGraph(true); });
    // TODO: move job queue into Core; that removes the lifetime issue and this capture
    runFn = [q = std::move(queue)]() mutable { Offline::run(q); };
}

void Application::buildRenderGraph(bool offline) {
    ShaderPlugin::regenerateAllDispatch();

    RenderGraphBuilder builder;
    RenderResources resources = Core::getCoreRenderer().initGraph(builder);
    RenderResources previewResources;
    if (!offline) {
        Editor::getEditorRenderer().initGraph(builder, resources);
        previewResources =
            Editor::getMaterialPreview().initGraph(builder, Core::getCoreRenderer().getLensImageHandle());
    }

    for (const std::string& error : Core::getEngine().rebuildGraph(builder))
        Log::error(ShaderSourceMap::remapError(error));

    if (!offline) {
        Editor::getEditorRenderer().registerImGuiTextures();
        Editor::getMaterialPreview().onGraphCompiled(previewResources);
    }
    Core::getScene().setGpuBufferHandles(resources.sceneHandles);
}

void Application::initScene(const std::string& sceneFile) {
    Core::getScene().init();

    ecs::ComponentUiRegistry::init();

    SceneSerializer::load(Core::getScene(), sceneFile);
}
