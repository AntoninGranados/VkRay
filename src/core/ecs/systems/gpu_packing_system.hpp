#pragma once

#include <vector>

#include "VkSmol/graph/builder_resource.hpp"

#include "core/ecs/components/component_type.hpp"
#include "core/ecs/registry.hpp"

struct SceneGpuBuffers {
    BufferHandle vertex;
    BufferHandle index;
    BufferHandle bvh;
    BufferHandle mesh;
    BufferHandle material;
    BufferHandle pluginParams;
    BufferHandle object;
    BufferHandle light;
    BufferHandle motion;
    BufferHandle liveMotion;
};

namespace ecs {

const std::vector<const ComponentType*>& objectTypeOrder();

void meshPackingSystem(Registry& registry);
void meshInstancePackingSystem(Registry& registry);
void materialPackingSystem(Registry& registry);
void objectPackingSystem(Registry& registry);
void lightPackingSystem(Registry& registry);

} // namespace ecs
