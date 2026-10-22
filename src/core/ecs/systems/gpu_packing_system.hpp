#pragma once

#include <vector>

#include "VkSmol/graph/builder_resource.hpp"

#include "core/ecs/components/component_type.hpp"
#include "core/ecs/registry.hpp"

struct SceneGpuBuffers {
    VkSmol::BufferHandle vertex;
    VkSmol::BufferHandle index;
    VkSmol::BufferHandle bvh;
    VkSmol::BufferHandle mesh;
    VkSmol::BufferHandle material;
    VkSmol::BufferHandle pluginParams;
    VkSmol::BufferHandle object;
    VkSmol::BufferHandle light;
    VkSmol::BufferHandle motion;
    VkSmol::BufferHandle liveMotion;
};

namespace ecs {

const std::vector<const ComponentType*>& objectTypeOrder();

void meshPackingSystem(Registry& registry);
void meshInstancePackingSystem(Registry& registry);
void materialPackingSystem(Registry& registry);
void objectPackingSystem(Registry& registry);
void lightPackingSystem(Registry& registry);

} // namespace ecs
