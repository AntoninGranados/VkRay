#pragma once

#include "../registry.hpp"

namespace ecs {

void cameraPreUpdateSystem(Registry& registry);
void cameraPostUpdateSystem(Registry& registry);

} // namespace ecs
