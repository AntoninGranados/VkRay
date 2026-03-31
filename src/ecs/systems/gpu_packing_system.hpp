#pragma once

#include "../registry.hpp"
#include "app/app_context.hpp"

struct FrameContext;

namespace ecs {

//! these are not really a "system" of the ECS as they only operates on the scene data
void spherePackingSystem(Registry& registry, AppContext& ctx, const FrameContext& frame);
void planePackingSystem(Registry& registry, AppContext& ctx, const FrameContext& frame);
void boxPackingSystem(Registry& registry, AppContext& ctx, const FrameContext& frame);
void meshPackingSystem(Registry& registry, AppContext& ctx, const FrameContext& frame);
void materialPackingSystem(Registry& registry, AppContext& ctx, const FrameContext& frame);
void objectPackingSystem(Registry& registry, AppContext& ctx, const FrameContext& frame);
void lightPackingSystem(Registry& registry, AppContext& ctx, const FrameContext& frame);

} // namespace ecs
