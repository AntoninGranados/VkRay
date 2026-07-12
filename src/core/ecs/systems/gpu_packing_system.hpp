#pragma once

#include "core/ecs/registry.hpp"

struct FrameContext;

namespace ecs {

//! these are not really a "system" of the ECS as they only operates on the scene data
void spherePackingSystem(Registry& registry, const FrameContext& frame);
void planePackingSystem(Registry& registry, const FrameContext& frame);
void boxPackingSystem(Registry& registry, const FrameContext& frame);
void quadPackingSystem(Registry& registry, const FrameContext& frame);
void meshPackingSystem(Registry& registry, const FrameContext& frame);
void materialPackingSystem(Registry& registry, const FrameContext& frame);
void objectPackingSystem(Registry& registry, const FrameContext& frame);
void lightPackingSystem(Registry& registry, const FrameContext& frame);

} // namespace ecs
