#pragma once

#include "core/ecs/entity.hpp"

namespace ecs {

void requestMeshSimplify(Entity meshEntity, float ratio);
void applyMeshSimplification(Entity meshEntity);
void revertMeshSimplification(Entity meshEntity);

} // namespace ecs
