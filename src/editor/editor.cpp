#include "editor.hpp"

#include <algorithm>
#include <chrono>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "core/core.hpp"
#include "core/export_service.hpp"

Editor& Editor::get() {
    static Editor instance;
    return instance;
}

void Editor::init() { get().inputHandler.initCallbacks(); }

EditorUi&       Editor::getUi()             { return get().ui; }
InputHandler&   Editor::getInputHandler()   { return get().inputHandler; }
EditorRenderer& Editor::getEditorRenderer() { return get().editorRenderer; }

std::optional<ecs::Entity> Editor::getSelectedEntity(){ return get().selectedEntity; }
void Editor::setSelectedEntity(ecs::Entity entity) { get().selectedEntity = entity; }
void Editor::clearSelectedEntity() { get().selectedEntity.reset(); }

void Editor::stepAnimation(float deltaTime) {
    if (!Core::getAnimation().isPaused()) Core::getAnimation().step(deltaTime);
}

void Editor::handleViewportResize() {
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
            get().editorRenderer.resize(Core::getCoreRenderer().getRenderExtent(), vpExtent);
        } else {
            get().editorRenderer.registerImGuiTextures();
        }
        Core::markDirty();
    }
}

void Editor::handleRenderModeCompletion() {
    if (Core::getRenderMode() == RenderMode::Preview || Core::isDirty()) return;
    if (!Core::getCoreRenderer().isRenderFinished()) return;

    if (Core::getRenderMode() == RenderMode::RenderAnimation) {
        const auto cacheDir = Core::getParameters().get<std::filesystem::path>("renderer/output/frame_cache");
        Core::getCoreRenderer().saveCapture(ExportService::buildAnimationFramePath(Core::getAnimation().getFrame(), cacheDir));
        Core::getAnimation().stepFixed();
        if (Core::getAnimation().getFrame() == 0) {
            ExportService::convertFramesToVideo(Core::getOutputPath(), cacheDir);
            get().ui.restoreToggledState();
            Core::setRenderMode(RenderMode::Preview);
        }
    } else {
        Core::getCoreRenderer().saveCapture(Core::getOutputPath());
        get().ui.restoreToggledState();
        Core::setRenderMode(RenderMode::Preview);
    }

    Core::markDirty();
}

void Editor::run() {
    auto startTime = std::chrono::high_resolution_clock::now();

    while (!Core::getEngine().shouldTerminate()) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        const float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        startTime = currentTime;

        stepAnimation(deltaTime);

        get().inputHandler.pollEvents();
        get().inputHandler.handle(deltaTime);

        handleViewportResize();
        handleRenderModeCompletion();

        Core::renderFrame([&](FrameContext& frameContext) {
            get().editorRenderer.render(frameContext);
            Core::getEngine().present();
        });
    }
}
