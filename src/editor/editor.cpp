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
    get().materialPreview.init();
    get().editorRenderer.init();
    Core::addStage(get().materialPreview);
    Core::addStage(get().editorRenderer);
}

EditorUi& Editor::getUi() { return get().ui; }
EditorRenderer& Editor::getEditorRenderer() { return get().editorRenderer; }
MaterialPreview& Editor::getMaterialPreview() { return get().materialPreview; }
TexturePreview& Editor::getTexturePreview() { return get().texturePreview; }

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
    camera = scene.createDefaultCamera("Viewport Camera", scene.getInternalsRoot());
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
    glfwGetWindowContentScale(Core::getWindow().getWindow(), &xscale, &yscale);
    VkExtent2D vpExtent = {static_cast<uint32_t>(vpSize.x * xscale), static_cast<uint32_t>(vpSize.y * yscale)};
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
        Core::markRenderDirty();
    }
}

void Editor::handleRenderModeCompletion() {
    if (Core::getRenderMode() == RenderMode::Preview || Core::isRenderDirty()) return;
    if (!Core::getCoreRenderer().isRenderFinished()) return;

    if (Core::getRenderMode() == RenderMode::RenderAnimation) {
        const auto cacheDir = Core::getParameters().get<std::filesystem::path>("renderer/output/frame_cache");
        ExportService::save(Core::getCoreRenderer(),
                            ExportService::buildAnimationFramePath(Core::getAnimation().getFrame(), cacheDir));
        Core::getAnimation().stepFixed();
        if (Core::getAnimation().getFrame() == 0) {
            ExportService::convertFramesToVideo(Core::getOutputPath(), cacheDir, Core::getAnimation().getFps());
            Core::setRenderMode(RenderMode::Preview);
        }
    } else {
        ExportService::save(Core::getCoreRenderer(), Core::getOutputPath());
        Core::setRenderMode(RenderMode::Preview);
    }

    Core::markRenderDirty();
}

void Editor::updateFrustumScale() {
    const ImVec2 viewportSize = get().ui.getViewportSize();
    const float viewportAspect = viewportSize.y > 0.0f ? viewportSize.x / viewportSize.y : 1.0f;
    Scene& scene = Core::getScene();
    Core::getCoreRenderer().setFrustumScale(
        EditorCameraPreview::computeFrustumScale(scene.getRegistry(), *scene.getActiveCamera(), viewportAspect));
}

void Editor::run() {
    auto startTime = std::chrono::high_resolution_clock::now();

    while (!Core::getWindow().shouldClose()) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        const float deltaTime =
            std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        startTime = currentTime;

        stepAnimation(deltaTime);

        get().inputHandler.pollEvents();
        get().inputHandler.handle(deltaTime);

        handleViewportResize();
        handleRenderModeCompletion();
        updateFrustumScale();
        if (Core::getParameters().get<ViewportShading>("renderer/viewport/shading") !=
            get().editorRenderer.getShading())
            Core::requestGraphRebuild();

        Core::renderFrame();
    }
    Core::getEngine().waitIdle();
}
