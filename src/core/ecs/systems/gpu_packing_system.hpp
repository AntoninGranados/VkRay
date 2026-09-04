#pragma once

#include <vector>

#include "core/ecs/components/component_type.hpp"
#include "core/ecs/registry.hpp"

namespace ecs {

const std::vector<const ComponentType*>& objectTypeOrder();

void spherePackingSystem(Registry& registry);
void planePackingSystem(Registry& registry);
void boxPackingSystem(Registry& registry);
void quadPackingSystem(Registry& registry);
void meshPackingSystem(Registry& registry);
void materialPackingSystem(Registry& registry);
void objectPackingSystem(Registry& registry);
void lightPackingSystem(Registry& registry);

} // namespace ecs
