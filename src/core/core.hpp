#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "VkSmol/engine.hpp"
#include "VkSmol/graph/render_graph.hpp"
#include "VkSmol/present/window.hpp"

#include "core/animation/animation_clock.hpp"
#include "core/fields/parameters.hpp"
#include "core/render/pathtrace_renderer.hpp"
#include "core/render/render_stage.hpp"
#include "core/render_structures.hpp"
#include "core/scene/scene.hpp"
#include "utils/file_watcher.hpp"

class Core {
public:
    static void init(const std::optional<std::string>& windowTitle, uint32_t version);
    static void loadParameters();

    static VkSmol::Engine& getEngine() { return get().engine; }
    static VkSmol::Window& getWindow() { return *get().window; }
    static VkSmol::RenderGraph& getGraph() { return get().graph; }
    static AnimationClock& getAnimation() { return get().animation; }
    static ParameterRegistry& getParameters() { return get().parameters; }
    static Scene& getScene() { return get().coreRenderer.getScene(); }
    static PathtraceRenderer& getCoreRenderer() { return get().coreRenderer; }
    static VkSmol::ImageHandle getLensImageHandle() { return get().lensImageHandle; }
    static VkSmol::BufferHandle getBlueNoiseBufferHandle() { return get().blueNoiseBufferHandle; }
    static FileWatcher& getFileWatcher() { return get().fileWatcher; }

    static RenderMode getRenderMode() { return get().renderMode; }
    static void setRenderMode(RenderMode m) { get().renderMode = m; }

    static void markRenderDirty() { get().renderDirty = true; }
    static bool isRenderDirty() { return get().renderDirty; }
    static bool consumeRenderDirty();

    static void addStage(RenderStage& stage) { get().stages.push_back(&stage); }
    // TODO: make it non blocking (compile/build in the background and replace when finished)
    static void buildGraph();
    static void requestGraphRebuild() { get().graphRebuildRequested = true; }

    static void requestResize(int width, int height) {
        get().targetExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    }
    static bool consumeResize();

    static void renderFrame();

    static const std::filesystem::path& getOutputPath() { return get().outputPath; }
    static void setOutputPath(std::filesystem::path p) { get().outputPath = std::move(p); }

    static void startRender();
    static void startRenderAnim();

private:
    static constexpr uint32_t kWindowWidth = 1280;
    static constexpr uint32_t kWindowHeight = 720;

    Core(const std::optional<std::string>& windowTitle, uint32_t version);
    static Core& get() { return *instance; }

    static void updateAnimationDirty(Core& c);
    static int getTargetSampleCount(Core& c);

    inline static Core* instance = nullptr;

    std::optional<VkSmol::Window> window;
    VkSmol::Engine engine;
    VkSmol::RenderGraph graph;
    ParameterRegistry parameters;
    AnimationClock animation{24 * 5, 24.0f};
    VkSmol::ImageHandle lensImageHandle;
    VkSmol::BufferHandle blueNoiseBufferHandle;
    PathtraceRenderer coreRenderer;
    FileWatcher fileWatcher;
    RenderMode renderMode = RenderMode::Preview;
    bool renderDirty = false;
    bool graphRebuildRequested = false;
    std::vector<RenderStage*> stages;
    VkExtent2D targetExtent = {};
    std::filesystem::path outputPath;
};
