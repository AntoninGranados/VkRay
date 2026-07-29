#pragma once

#include "core/ecs/registry.hpp"

namespace ecs {

void cameraPreUpdateSystem(Registry& registry);
void cameraPostUpdateSystem(Registry& registry);

} // namespace ecs
