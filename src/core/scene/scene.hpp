#pragma once

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "VkSmol/engine.hpp"

#include "core/animation/animation_store.hpp"
#include "core/camera/camera.hpp"
#include "core/scene/asset/mesh.hpp"
#include "core/ecs/registry.hpp"
#include "core/ecs/entity.hpp"
#include "core/ecs/components.hpp"
#include "core/ecs/system_scheduler.hpp"
#include "core/structures.hpp"
#include "core/scene/object.hpp"


struct SceneGpuBufferEntry {
    BufferHandle handle;
    size_t capacity = 0;
};

struct SceneGpuBuffers {
    SceneGpuBufferEntry sphere;
    SceneGpuBufferEntry plane;
    SceneGpuBufferEntry box;
    SceneGpuBufferEntry quad;
    SceneGpuBufferEntry vertex;
    SceneGpuBufferEntry index;
    SceneGpuBufferEntry bvh;
    SceneGpuBufferEntry mesh;
    SceneGpuBufferEntry material;
    SceneGpuBufferEntry object;
    SceneGpuBufferEntry light;
};

class Scene {
public:
    void init();
    void destroy();
    void clear();

    ecs::Entity pushMaterial(const ecs::ComponentType& bsdfType, std::string name = {});
    void pushSphere(std::string name, glm::vec3 center, float radius, uint32_t materialHandle = 0);
    void pushPlane(std::string name, glm::vec3 point, glm::vec3 normal, uint32_t materialHandle = 0);
    void pushBox(std::string name, glm::vec3 cornerMin, glm::vec3 cornerMax, uint32_t materialHandle = 0);
    void pushQuad(std::string name, glm::vec3 center, glm::vec3 normal, glm::vec2 scale, float rotation = 0.0f, uint32_t materialHandle = 0);
    void pushMesh(std::string name, const std::string& path, const glm::mat4& transform, uint32_t materialHandle = 0, bool smoothShading = false);
    void pushMesh(std::string name, MeshHandle meshHandle, const glm::mat4& transform, uint32_t materialHandle = 0);
    void pushCamera(std::string name, const glm::mat4& transform);
    
    void bakePhysics();
    bool isPhysicsBakeInProgress() const;
    int getPhysicsBakeCurrentFrame() const;
    int getPhysicsBakeTotalFrames() const;
    
    void runPreRender()                         { preUpdateScheduler.run(registry); }
    void runOnRender(const FrameContext& frame) { onRenderScheduler.run(registry, frame); }
    void runPostRender()                        { postUpdateScheduler.run(registry); }
    
    ecs::Registry& getRegistry() { return registry; }
    const ecs::Registry& getRegistry() const { return registry; }

    Camera& getCamera() { return camera; }
    AnimationStore& getAnimationStore() { return animationStore; }
    
    SceneGpuBuffers& getBuffers() { return gpuBuffers; }
    const SceneGpuBuffers& getBuffers() const { return gpuBuffers; }
    void setGpuBufferHandles(SceneGpuBuffers handles);

    std::vector<MeshAsset>& getMeshAssets() { return meshAssets; }
    const std::vector<ecs::Entity>& getEntities() const { return entities; }
    std::vector<ecs::Entity>& getEntities() { return entities; }

    bool checkUpdate();
    void update() { updated = true; ++generation; }
    size_t getGeneration() const { return generation; }

private:
    SceneGpuBuffers gpuBuffers;

    ecs::Registry registry;
    ecs::SystemScheduler<> preUpdateScheduler, postUpdateScheduler;
    ecs::SystemScheduler<const FrameContext&> onRenderScheduler;
    
    std::vector<ecs::Entity> entities;
    std::vector<MeshAsset> meshAssets;

    Camera camera = Camera(glm::vec3(0.0f, 0.0f, -10.0f));
    AnimationStore animationStore;

    bool updated = false;
    size_t generation = 1;

    void initSystems();

    ecs::Entity createNamedEntity(std::string name);
    void addMaterialRef(ecs::Entity e, uint32_t materialHandle);
    void addTransformFromMatrix(ecs::Entity e, const glm::mat4& transform);
    void resetSceneState();
    void addDefaultAssets();

};
