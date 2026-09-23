#pragma once

#include <cstdint>
#include <vector>

#include "core/ecs/entity.hpp"
#include "core/ecs/registry.hpp"

struct GpuMotionSample;
namespace ecs { class Component; }

namespace ecs {

uint32_t bakeMotionSamples(Registry& registry, Entity entity, Component& transform,
                            std::vector<GpuMotionSample>& outMotion);

void bakeLiveTransform(Component& transform, std::vector<GpuMotionSample>& outMotion);

} // namespace ecs
