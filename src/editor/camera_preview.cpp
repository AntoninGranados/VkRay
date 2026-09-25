#include "camera_preview.hpp"

#include "core/camera/camera.hpp"
#include "core/core.hpp"
#include "core/ecs/components/camera.hpp"
#include "core/fields/parameters.hpp"
#include "core/scene/scene.hpp"

namespace {

constexpr float kPreviewMinFrameFraction = 0.9f;

float widenForPreview(const ecs::Registry& registry, ecs::Entity camera, float aspect, float trueValue) {
    if (&registry != &Core::getScene().getRegistry() || Core::getRenderMode() != RenderMode::Preview || !Core::getScene().isUsingSceneCamera() || camera != Core::getScene().getCamera())
        return trueValue;

    const ecs::Component& c = registry.get(camera, ecs::Camera);
    const ecs::CameraSensorFit fit = static_cast<ecs::CameraSensorFit>(c.get<int>("sensor_fit"));
    const glm::ivec2 outputSize = Core::getParameters().get<glm::ivec2>("renderer/output/render_size");
    const float renderAspect = outputSize.y > 0 ? static_cast<float>(outputSize.x) / static_cast<float>(outputSize.y) : aspect;

    const glm::vec2 trueHalf = applySensorFit(fit, renderAspect, trueValue);
    const glm::vec2 fitScale = applySensorFit(fit, aspect, 1.0f);
    return glm::max(
        trueHalf.x / (kPreviewMinFrameFraction * fitScale.x),
        trueHalf.y / (kPreviewMinFrameFraction * fitScale.y)
    );
}

} // namespace

void EditorCameraPreview::install() {
    setFrustumHook(widenForPreview);
}

glm::vec2 EditorCameraPreview::frameExtent(const ecs::Registry& registry, ecs::Entity camera, float viewportAspect, float renderAspect) {
    const CameraFrustum trueFrustum = computeFrustum(registry, camera, renderAspect, false);
    const CameraFrustum widenedFrustum = computeFrustum(registry, camera, viewportAspect, true);
    return trueFrustum.half / widenedFrustum.half;
}
