#pragma once

#include "../registry.hpp"
#include "app/app_context.hpp"

namespace ecs {

//! these are not really a "system" of the ECS as they only operates on the scene data
void spherePackingSystem(Registry& registry, AppContext& ctx);
void planePackingSystem(Registry& registry, AppContext& ctx);
void boxPackingSystem(Registry& registry, AppContext& ctx);
void meshPackingSystem(Registry& registry, AppContext& ctx);
void materialPackingSystem(Registry& registry, AppContext& ctx);
void objectPackingSystem(Registry& registry, AppContext& ctx);
void lightPackingSystem(Registry& registry, AppContext& ctx);

} // namespace ecs
