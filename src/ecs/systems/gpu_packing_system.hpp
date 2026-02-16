#pragma once

#include "../registry.hpp"
#include "app/app_context.hpp"

namespace ecs {

void spherePackingSystem(Registry& registry, AppContext& ctx);
void planePackingSystem(Registry& registry, AppContext& ctx);
void boxPackingSystem(Registry& registry, AppContext& ctx);
void meshPackingSystem(Registry& registry, AppContext& ctx);
void materialPackingSystem(Registry& registry, AppContext& ctx);    //! this is not really a "system" of the ECS as it only operates on the scene data
void objectPackingSystem(Registry& registry, AppContext& ctx);
void lightPackingSystem(Registry& registry, AppContext& ctx);

} // namespace ecs
