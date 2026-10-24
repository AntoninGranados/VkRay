#include "camera_preview.hpp"

#include "core/camera/camera.hpp"
#include "core/core.hpp"
#include "core/ecs/components/camera.hpp"
#include "core/fields/parameters.hpp"
#include "editor/editor.hpp"

namespace {

constexpr float kPreviewMinFrameFraction = 0.9f;

} // namespace

float EditorCameraPreview::computeFrustumScale(const ecs::Registry& registry, ecs::Entity camera,
                                               float viewportAspect) {
    if (Core::getRenderMode() != RenderMode::Preview || !Editor::isUsingSceneCamera()) return 1.0f;

    const ecs::Component& c = registry.get(camera, ecs::Camera);
    const ecs::CameraSensorFit fit = static_cast<ecs::CameraSensorFit>(c.get<int>("sensor_fit"));
    const glm::ivec2 outputSize = Core::getParameters().get<glm::ivec2>("renderer/output/render_size");
    const float renderAspect =
        outputSize.y > 0 ? static_cast<float>(outputSize.x) / static_cast<float>(outputSize.y) : viewportAspect;

    const glm::vec2 renderFit = applySensorFit(fit, renderAspect, 1.0f);
    const glm::vec2 viewportFit = applySensorFit(fit, viewportAspect, 1.0f);
    return glm::max(renderFit.x / (kPreviewMinFrameFraction * viewportFit.x),
                    renderFit.y / (kPreviewMinFrameFraction * viewportFit.y));
}

glm::vec2 EditorCameraPreview::frameExtent(const ecs::Registry& registry, ecs::Entity camera, float viewportAspect,
                                           float renderAspect, float frustumScale) {
    const CameraFrustum trueFrustum = computeFrustum(registry, camera, renderAspect);
    const CameraFrustum widenedFrustum = computeFrustum(registry, camera, viewportAspect, frustumScale);
    return trueFrustum.half / widenedFrustum.half;
}
