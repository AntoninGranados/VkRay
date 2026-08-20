#pragma once

#include "core/ecs/entity.hpp"
#include "core/ecs/registry.hpp"

namespace ecs {

void requestMeshSimplify(Registry& registry, Entity meshEntity, float ratio);
void applyMeshSimplification(Registry& registry, Entity meshEntity);
void revertMeshSimplification(Registry& registry, Entity meshEntity);

} // namespace ecs
