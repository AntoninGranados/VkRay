#pragma once

#include "../registry.hpp"
#include "../../app_context.hpp"

namespace ecs {

void spherePackingSystem(Registry& registry, AppContext& ctx);
void planePackingSystem(Registry& registry, AppContext& ctx);
void boxPackingSystem(Registry& registry, AppContext& ctx);
void meshPackingSystem(Registry& registry, AppContext& ctx);

} // namespace ecs
