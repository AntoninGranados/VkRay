#pragma once

#include "core/ecs/registry.hpp"

namespace ecs {

void cameraActivationSystem(Registry& registry);

void cameraDrawingSystem(Registry& registry);

void cameraControlSystem(Registry& registry);

void cameraCursorCallback(Registry& registry, ecs::Entity camera, double x, double y);
void cameraScrollCallback(Registry& registry, ecs::Entity camera, double xoffset, double yoffset);

} // namespace ecs
