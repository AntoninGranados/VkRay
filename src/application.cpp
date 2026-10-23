#include "application.hpp"

#include <cstdlib>
#include <filesystem>
#include <format>
#include <string>
#include <string_view>

#include "VkSmol/graph/render_graph.hpp"

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

#include "utils/log.hpp"
#include "utils/resources.hpp"

Application::Application(int argc, char* argv[]) {
    if (argc >= 3 && std::string_view(argv[1]) == "--job") {
        initOfflineMode(argv[2]);
    } else {
        initEditorMode();
    }
}

int Application::run() { return runFn ? runFn() : EXIT_SUCCESS; }

void Application::initEditorMode() {
    Core::init(std::format("VkRay [{}]", VK_RAY_VERSION_STRING), VK_RAY_VERSION);

    Editor::init();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;

    float xscale, yscale;
    glfwGetWindowContentScale(Core::getWindow().getWindow(), &xscale, &yscale);

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

    ecs::ComponentUiRegistry::init();
    Editor::loadScene(kDefaultScene);
    Core::installGraphBuilder([this] { buildRenderGraph(false); });
    runFn = [] {
        Editor::run();
        return EXIT_SUCCESS;
    };
}

void Application::initOfflineMode(const std::string& jobFile) {
    JobQueue queue;
    try {
        queue = JobQueue::fromFile(jobFile);
    } catch (const std::exception& e) {
        Log::error("Application", std::format("Failed to load job queue `{}`: {}", jobFile, e.what()));
        runFn = [] { return EXIT_FAILURE; };
        return;
    }
    if (queue.isEmpty()) return;

    Core::init(std::nullopt, VK_RAY_VERSION);

    SceneSerializer::load(Core::getScene(), kDefaultScene);
    Core::installGraphBuilder([this] { buildRenderGraph(true); });
    runFn = [queue = std::move(queue)]() mutable { return Offline::run(queue); };
}

void Application::buildRenderGraph(bool offline) {
    VkSmol::RenderGraph& graph = Core::getGraph();
    graph.reset();
    Core::getCoreRenderer().addPasses(graph);
    if (!offline) {
        Editor::getMaterialPreview().addPasses(graph);
        Editor::getEditorRenderer().addPasses(graph);
    }
    graph.bake();
}
