#pragma once

#include <optional>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "VkSmol/engine.hpp"

#include "core/animation/animation_store.hpp"
#include "core/camera/camera.hpp"
#include "core/scene/asset/mesh.hpp"
#include "core/ecs/registry.hpp"
#include "core/ecs/components.hpp"
#include "core/ecs/system_scheduler.hpp"
#include "core/structures.hpp"


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
    ecs::Entity pushMeshAsset(std::string name, const std::string& path, bool smooth = false);
    void pushSphere(std::string name, glm::vec3 center, float radius, std::optional<ecs::Entity> materialEntity = {});
    void pushPlane(std::string name, glm::vec3 point, glm::vec3 normal, std::optional<ecs::Entity> materialEntity = {});
    void pushBox(std::string name, glm::vec3 cornerMin, glm::vec3 cornerMax, std::optional<ecs::Entity> materialEntity = {});
    void pushQuad(std::string name, glm::vec3 center, glm::vec3 normal, glm::vec2 scale, float rotation = 0.0f, std::optional<ecs::Entity> materialEntity = {});
    void pushMesh(std::string name, const std::string& path, const glm::mat4& transform, std::optional<ecs::Entity> materialEntity = {}, bool smooth = false);
    void pushMesh(std::string name, ecs::Entity meshAssetEntity, const glm::mat4& transform, std::optional<ecs::Entity> materialEntity = {});
    void pushCamera(std::string name, const glm::mat4& transform);

    const std::vector<ecs::Entity>& getChildren(ecs::Entity parent) const;

    ecs::Entity getMaterialsRoot() const { return materialsRoot; }
    ecs::Entity getAssetsRoot() const { return assetsRoot; }
    ecs::Entity getObjectsRoot() const { return objectsRoot; }
    ecs::Entity getDefaultMaterial() const { return defaultMaterial; }
    ecs::Entity getDefaultMeshAsset() const { return defaultMeshAsset; }

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

    MeshAsset* getMeshAsset(ecs::Entity e);
    const MeshAsset* getMeshAsset(ecs::Entity e) const;

    bool checkUpdate();
    void update() { updated = true; ++generation; }
    void touch() { ++generation; }
    size_t getGeneration() const { return generation; }

private:
    SceneGpuBuffers gpuBuffers;

    ecs::Registry registry;
    ecs::SystemScheduler<> preUpdateScheduler, postUpdateScheduler;
    ecs::SystemScheduler<const FrameContext&> onRenderScheduler;

    ecs::Entity materialsRoot;
    ecs::Entity assetsRoot;
    ecs::Entity objectsRoot;
    ecs::Entity defaultMaterial;
    ecs::Entity defaultMeshAsset;

    Camera camera = Camera(glm::vec3(0.0f, 0.0f, -10.0f));
    AnimationStore animationStore;

    bool updated = false;
    size_t generation = 1;

    void initSystems();

    ecs::Entity createNamedEntity(std::string name, ecs::Entity parent = {});
    void addTransformFromMatrix(ecs::Entity e, const glm::mat4& transform);
    void resetSceneState();
    void addDefaultAssets();

};
