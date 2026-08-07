#pragma once

#include "core/ecs/registry.hpp"

class Camera;

namespace ecs {

void cameraPreUpdateSystem(Registry& registry);
void syncPreviewCameraToEntity(const ::Camera& cam, Registry& registry);

} // namespace ecs
