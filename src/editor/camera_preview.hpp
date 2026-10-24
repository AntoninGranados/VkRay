#pragma once

#include <glm/glm.hpp>

#include "core/ecs/entity.hpp"

namespace ecs {
class Registry;
}

namespace EditorCameraPreview {

float computeFrustumScale(const ecs::Registry& registry, ecs::Entity camera, float viewportAspect);

glm::vec2 frameExtent(const ecs::Registry& registry, ecs::Entity camera, float viewportAspect, float renderAspect,
                      float frustumScale);

} // namespace EditorCameraPreview
