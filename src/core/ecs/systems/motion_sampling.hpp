#pragma once

#include <cstdint>
#include <vector>

#include "core/ecs/entity.hpp"

class Scene;
struct GpuMotionSample;
namespace ecs {
class Component;
}

namespace ecs {

uint32_t bakeMotionSamples(Scene& scene, Entity entity, Component& transform, std::vector<GpuMotionSample>& outMotion);

void bakeLiveTransform(Component& transform, std::vector<GpuMotionSample>& outMotion);

bool isMotionBlurActive(Scene& scene);

} // namespace ecs
