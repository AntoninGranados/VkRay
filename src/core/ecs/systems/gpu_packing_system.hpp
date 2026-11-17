#pragma once

#include <initializer_list>
#include <vector>

#include "VkSmol/graph/builder_resource.hpp"

#include "core/ecs/components/component_type.hpp"
#include "core/ecs/registry.hpp"
#include "core/ecs/system_scheduler.hpp"

class Scene;

namespace VkSmol {
class Engine;
}

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
    std::vector<VkSmol::ImageHandle> textures;
    std::vector<VkSmol::ImageHandle> retiredTextures;
    bool sharesTextures = false;
    bool texturesChanged = false;
};

namespace ecs {

const std::vector<const ComponentType*>& objectTypeOrder();

class PackingSystem : public System {
protected:
    PackingSystem(Scene& scene, SceneGpuBuffers& buffers, VkSmol::Engine& engine,
                  std::vector<const ComponentType*> reads);

    static std::vector<const ComponentType*> join(std::initializer_list<std::vector<const ComponentType*>> lists);

    Scene& scene;
    SceneGpuBuffers& buffers;
    VkSmol::Engine& engine;
};

class TexturePackingSystem : public PackingSystem {
public:
    TexturePackingSystem(Scene& scene, SceneGpuBuffers& buffers, VkSmol::Engine& engine);
    void run(Registry& registry) override;
};

class MeshPackingSystem : public PackingSystem {
public:
    MeshPackingSystem(Scene& scene, SceneGpuBuffers& buffers, VkSmol::Engine& engine);
    void run(Registry& registry) override;
};

class MeshInstancePackingSystem : public PackingSystem {
public:
    MeshInstancePackingSystem(Scene& scene, SceneGpuBuffers& buffers, VkSmol::Engine& engine);
    void run(Registry& registry) override;
};

class MaterialPackingSystem : public PackingSystem {
public:
    MaterialPackingSystem(Scene& scene, SceneGpuBuffers& buffers, VkSmol::Engine& engine);
    void run(Registry& registry) override;
};

class ObjectPackingSystem : public PackingSystem {
public:
    ObjectPackingSystem(Scene& scene, SceneGpuBuffers& buffers, VkSmol::Engine& engine);
    void run(Registry& registry) override;
    bool forcesRun(Registry& registry) const override;
};

class LightPackingSystem : public PackingSystem {
public:
    LightPackingSystem(Scene& scene, SceneGpuBuffers& buffers, VkSmol::Engine& engine);
    void run(Registry& registry) override;
};

} // namespace ecs
