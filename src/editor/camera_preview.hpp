#pragma once

#include <glm/glm.hpp>

#include "core/ecs/entity.hpp"

namespace ecs { class Registry; }

namespace EditorCameraPreview {

void install();

glm::vec2 frameExtent(const ecs::Registry& registry, ecs::Entity camera, float viewportAspect, float renderAspect);

}
