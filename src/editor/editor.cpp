#include "editor.hpp"

#include <algorithm>
#include <chrono>
#include <unordered_map>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "core/core.hpp"
#include "core/ecs/components.hpp"
#include "core/export_service.hpp"

Editor& Editor::get() {
    static Editor instance;
    return instance;
}

void Editor::init() { get().inputHandler.initCallbacks(); }

EditorUi&       Editor::getUi()             { return get().ui; }
InputHandler&   Editor::getInputHandler()   { return get().inputHandler; }
EditorRenderer& Editor::getEditorRenderer() { return get().editorRenderer; }

void Editor::run() {
    auto startTime = std::chrono::high_resolution_clock::now();
    uint64_t lastSwapchainGeneration = 0;

    while (!Core::getEngine().shouldTerminate()) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        startTime = currentTime;

        if (!Core::getAnimation().isPaused()) Core::getAnimation().step(deltaTime);

        get().inputHandler.pollEvents();
        get().inputHandler.handle(deltaTime);

        ImVec2 vpSize = get().ui.getViewportSize();
        float xscale = 1.0f, yscale = 1.0f;
        glfwGetWindowContentScale(
            static_cast<GLFWwindow*>(Core::getPlatform().getNativeWindowHandle()),
            &xscale, &yscale);
        VkExtent2D vpExtent = {
            static_cast<uint32_t>(vpSize.x * xscale),
            static_cast<uint32_t>(vpSize.y * yscale)
        };
        if (Core::getRenderMode() == RenderMode::Preview && vpExtent.width > 0 && vpExtent.height > 0) {
            const int pixelScale = std::max(1, Core::getParameters().get<int>("renderer/viewport/pixel_scale"));
            Core::requestResize(vpExtent.width / pixelScale, vpExtent.height / pixelScale);
        }

        if (Core::consumeResize()) {
            if (Core::getRenderMode() == RenderMode::Preview) {
                VkExtent2D e = Core::getCoreRenderer().getRenderExtent();
                get().editorRenderer.resize(e.width, e.height);
            } else {
                get().editorRenderer.registerImGuiTextures();
            }
            Core::requestAccumulationRestart();
        }
        Core::consumeAccumulationRestart();

        bool shouldSave = false;
        std::filesystem::path savePath;
        bool toVideo = false;

        if (Core::getRenderMode() != RenderMode::Preview && !Core::isAccumulationRestartPending()) {
            if (Core::getCoreRenderer().isRenderFinished()) {
                shouldSave = true;

                if (Core::getRenderMode() == RenderMode::RenderAnimation) {
                    const auto cacheDir = Core::getParameters().get<std::filesystem::path>("renderer/output/frame_cache");
                    savePath = ExportService::buildAnimationFramePath(Core::getAnimation().getFrame(), cacheDir);
                    Core::getAnimation().stepFixed();
                    if (Core::getAnimation().getFrame() == 0) {
                        toVideo = true;
                        get().ui.restoreToggledState();
                        Core::setRenderMode(RenderMode::Preview);
                    }
                } else {
                    savePath = Core::getOutputPath();
                    get().ui.restoreToggledState();
                    Core::setRenderMode(RenderMode::Preview);
                }

                Core::requestAccumulationRestart();
            }
        }

        const SceneSelection& sel = get().ui.getSelection();
        int flatIdx = -1;
        if (sel.entity >= 0) {
            const ecs::Entity e = Core::getScene().getEntities()[static_cast<size_t>(sel.entity)];
            const ScenePackingMaps& maps = Core::getScene().getPackingMaps();
            const ecs::Registry& registry = Core::getScene().getRegistry();
            int i = 0;
            auto check = [&](const ecs::ComponentType& type, const std::unordered_map<ecs::Entity, int>& m) {
                for (const auto& ent : registry.storage(type).entities()) {
                    if (m.find(ent) == m.end()) continue;
                    if (ent == e) { flatIdx = i; return; }
                    i++;
                }
            };
            check(ecs::Sphere, maps.sphereId);
            if (flatIdx < 0) check(ecs::Plane, maps.planeId);
            if (flatIdx < 0) check(ecs::Box, maps.boxId);
            if (flatIdx < 0) check(ecs::Quad, maps.quadId);
            if (flatIdx < 0) check(ecs::MeshRef, maps.meshId);
        }
        Core::getCoreRenderer().setSelectedObjectId(flatIdx);

        Core::renderFrame([&](FrameContext& frameContext) {
            if (frameContext.swapchainGeneration != lastSwapchainGeneration)
                lastSwapchainGeneration = frameContext.swapchainGeneration;

            get().editorRenderer.render(frameContext);

            if (shouldSave) {
                Core::getCoreRenderer().saveCapture(savePath);
                if (toVideo) ExportService::convertFramesToVideo(Core::getOutputPath(), Core::getParameters().get<std::filesystem::path>("renderer/output/frame_cache"));
            }

            Core::getEngine().present();
        });
    }
}
