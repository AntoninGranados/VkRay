#pragma once

#include "../registry.hpp"
#include "../../app_context.hpp"

namespace ecs {

void cameraDrawingSystem(Registry& registry, AppContext& ctx);
void cameraPreUpdateSystem(Registry& registry, AppContext& ctx);
void cameraPostUpdateSystem(Registry& registry, AppContext& ctx);

} // namespace ecs
