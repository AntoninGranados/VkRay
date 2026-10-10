#include "editor.hpp"

#include <algorithm>
#include <chrono>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include "core/core.hpp"
#include "core/ecs/components/camera.hpp"
#include "core/ecs/components/core.hpp"
#include "core/export_service.hpp"
#include "core/scene/scene.hpp"
#include "core/scene/scene_serializer.hpp"
#include "editor/camera_preview.hpp"

Editor& Editor::get() {
    static Editor instance;
    return instance;
}

void Editor::init() {
    get().inputHandler.initCallbacks();
    EditorCameraPreview::install();
}

void Editor::terminate() {
    get().editorRenderer.destroy();
    get().materialPreview.destroy();
}

EditorUi& Editor::getUi() { return get().ui; }
EditorRenderer& Editor::getEditorRenderer() { return get().editorRenderer; }
MaterialPreview& Editor::getMaterialPreview() { return get().materialPreview; }

std::optional<ecs::Entity> Editor::getSelectedEntity() { return get().selectedEntity; }
void Editor::setSelectedEntity(ecs::Entity entity) { get().selectedEntity = entity; }
void Editor::clearSelectedEntity() { get().selectedEntity.reset(); }

void Editor::selectEntity(std::optional<ecs::Entity> entity) {
    const bool changed = get().selectedEntity != entity;
    if (entity.has_value())
        setSelectedEntity(*entity);
    else
        clearSelectedEntity();

    if (changed) Core::markRenderDirty();
}

bool Editor::loadScene(const std::string& path) {
    Scene& scene = Core::getScene();
    const bool loaded = SceneSerializer::load(scene, path);

    if (!scene.getRegistry().isAlive(get().viewportCamera)) createViewportCamera();
    if (!scene.getActiveCamera()) scene.setActiveCamera(get().viewportCamera);
    return loaded;
}

void Editor::createViewportCamera() {
    Scene& scene = Core::getScene();
    ecs::Registry& registry = scene.getRegistry();

    ecs::Entity& camera = get().viewportCamera;
    camera = scene.createNamedEntity("Viewport Camera", scene.getInternalsRoot());
    registry.add(camera, ecs::Camera);
    registry.get(camera, ecs::Transform).set<glm::vec3>("position", glm::vec3(0.0f, 0.0f, -10.0f));
    registry.get(camera, ecs::Transform).set<glm::vec3>("rotation", glm::vec3(0.0f, 180.0f, 0.0f));
    registry.add(camera, ecs::Locked);
}

bool Editor::isUsingSceneCamera() { return Core::getScene().getActiveCamera() != get().viewportCamera; }

void Editor::activateSceneCamera() {
    Scene& scene = Core::getScene();
    for (const ecs::Entity& entity : scene.getRegistry().storage(ecs::Camera).entities()) {
        if (entity == get().viewportCamera) continue;
        scene.setActiveCamera(entity);
        return;
    }
    scene.setActiveCamera(get().viewportCamera);
}

void Editor::stepAnimation(float deltaTime) {
    if (Core::getRenderMode() == RenderMode::Preview && !Core::getAnimation().isPaused())
        Core::getAnimation().step(deltaTime);
}

void Editor::handleViewportResize() {
    ImVec2 vpSize = get().ui.getViewportSize();
    float xscale = 1.0f, yscale = 1.0f;
    glfwGetWindowContentScale(static_cast<GLFWwindow*>(Core::getPlatform().getNativeWindowHandle()), &xscale, &yscale);
    VkExtent2D vpExtent = {static_cast<uint32_t>(vpSize.x * xscale), static_cast<uint32_t>(vpSize.y * yscale)};
    if (Core::getRenderMode() == RenderMode::Preview && vpExtent.width > 0 && vpExtent.height > 0) {
        const int pixelScale = std::max(1, Core::getParameters().get<int>("renderer/viewport/pixel_scale"));
        Core::requestResize(vpExtent.width / pixelScale, vpExtent.height / pixelScale);
    }

    if (Core::consumeResize()) {
        if (Core::getRenderMode() == RenderMode::Preview) {
            get().editorRenderer.resize(Core::getRenderExtent(), vpExtent);
        } else {
            get().editorRenderer.registerImGuiTextures();
        }
        Core::markRenderDirty();
    }
}

void Editor::handleRenderModeCompletion() {
    if (Core::getRenderMode() == RenderMode::Preview || Core::isRenderDirty()) return;
    if (!Core::isRenderFinished()) return;

    if (Core::getRenderMode() == RenderMode::RenderAnimation) {
        const auto cacheDir = Core::getParameters().get<std::filesystem::path>("renderer/output/frame_cache");
        Core::saveCapture(ExportService::buildAnimationFramePath(Core::getAnimation().getFrame(), cacheDir));
        Core::getAnimation().stepFixed();
        if (Core::getAnimation().getFrame() == 0) {
            ExportService::convertFramesToVideo(Core::getOutputPath(), cacheDir);
            Core::setRenderMode(RenderMode::Preview);
        }
    } else {
        Core::saveCapture(Core::getOutputPath());
        Core::setRenderMode(RenderMode::Preview);
    }

    Core::markRenderDirty();
}

void Editor::run() {
    auto startTime = std::chrono::high_resolution_clock::now();

    while (!Core::getEngine().shouldTerminate()) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        const float deltaTime =
            std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        startTime = currentTime;

        stepAnimation(deltaTime);

        get().inputHandler.pollEvents();
        get().inputHandler.handle(deltaTime);

        handleViewportResize();
        handleRenderModeCompletion();

        Core::renderFrame([&](FrameContext& frameContext) {
            get().editorRenderer.render(frameContext);
            get().materialPreview.tick();
            Core::getEngine().present();
        });
    }
}
