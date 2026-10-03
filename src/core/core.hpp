#pragma once

#include <filesystem>
#include <functional>

#include "VkSmol/engine.hpp"
#include "VkSmol/frame_context.hpp"
#include "VkSmol/platform/platform.hpp"

#include "core/animation/animation_clock.hpp"
#include "core/core_renderer.hpp"
#include "core/fields/parameters.hpp"
#include "core/render_structures.hpp"
#include "core/scene/scene.hpp"
#include "utils/file_watcher.hpp"

class Core {
public:
    static void init(Platform& platform, uint32_t version);
    static void loadParameters();
    static void terminate();

    static VkSmol& getEngine() { return get().engine; }
    static Platform& getPlatform() { return *get().platform; }
    static AnimationClock& getAnimation() { return get().animation; }
    static ParameterRegistry& getParameters() { return get().parameters; }
    static Scene& getScene() { return get().coreRenderer.getScene(); }
    static CoreRenderer& getCoreRenderer() { return get().coreRenderer; }
    static FileWatcher& getFileWatcher() { return get().fileWatcher; }

    static RenderMode getRenderMode() { return get().renderMode; }
    static void setRenderMode(RenderMode m) { get().renderMode = m; }

    static void markRenderDirty() { get().renderDirty = true; }
    static void restartAccumulation();
    static bool isRenderDirty() { return get().renderDirty; }
    static bool consumeRenderDirty();

    static void setTargetSampleCount(int count) { get().coreRenderer.setTargetSampleCount(count); }
    static uint32_t getSampleCount() { return get().coreRenderer.getSampleCount(); }
    static bool isRenderFinished() { return get().coreRenderer.isRenderFinished(); }
    static VkExtent2D getRenderExtent() { return get().coreRenderer.getRenderExtent(); }
    static void saveCapture(const std::filesystem::path& path) { get().coreRenderer.saveCapture(path); }

    static void installGraphBuilder(std::function<void()> fn) {
        Core& c = get();
        c.graphBuilder = std::move(fn);
        c.graphBuilder();
    }
    static void requestGraphRebuild() { get().graphRebuildRequested = true; }

    static void resize(int width, int height);
    static void requestResize(int width, int height) {
        get().targetExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    }
    static bool consumeResize();

    static void renderFrame(std::function<void(FrameContext&)> onRender = {});

    static const std::filesystem::path& getOutputPath() { return get().outputPath; }
    static void setOutputPath(std::filesystem::path p) { get().outputPath = std::move(p); }

    static void startRender();
    static void startRenderAnim();

private:
    Core() = default;
    static Core& get();

    static void updateAnimationDirty(Core& c);
    static void rebuildGraphIfRequested(Core& c);

    Platform* platform = nullptr;
    VkSmol engine;
    ParameterRegistry parameters;
    AnimationClock animation{24 * 5, 24.0f};
    CoreRenderer coreRenderer;
    FileWatcher fileWatcher;
    RenderMode renderMode = RenderMode::Preview;
    bool renderDirty = false;
    bool graphRebuildRequested = false;
    std::function<void()> graphBuilder;
    VkExtent2D targetExtent = {};
    std::filesystem::path outputPath;
};
