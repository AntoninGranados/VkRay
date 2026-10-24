#pragma once

#include <vector>

#include "VkSmol/graph/builder_resource.hpp"

#include "core/ecs/components/component_type.hpp"
#include "core/ecs/registry.hpp"

class Scene;

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

void meshPackingSystem(Scene& scene, const SceneGpuBuffers& buffers);
void meshInstancePackingSystem(Scene& scene, const SceneGpuBuffers& buffers);
void materialPackingSystem(Scene& scene, const SceneGpuBuffers& buffers);
void objectPackingSystem(Scene& scene, const SceneGpuBuffers& buffers);
void lightPackingSystem(Registry& registry, const SceneGpuBuffers& buffers);

} // namespace ecs
