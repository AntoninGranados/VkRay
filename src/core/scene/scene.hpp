#pragma once

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "VkSmol/engine.hpp"

#include "core/animation/animation_store.hpp"
#include "core/camera/camera.hpp"
#include "core/ecs/entity.hpp"
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

struct SceneRoots {
    ecs::Entity materialsRoot;
    ecs::Entity assetsRoot;
    ecs::Entity objectsRoot;
    ecs::Entity internalsRoot;
};

class Scene {
public:
    void init();
    void destroy();
    void clear();

    ecs::Entity createNamedEntity(std::string name, ecs::Entity parent = {});
    ecs::Entity loadMeshAsset(std::string name, const std::string& path, bool smooth = false);

    const std::vector<ecs::Entity>& getChildren(ecs::Entity parent) const;

    ecs::Entity getMaterialsRoot() const { return registry.ctx().get<SceneRoots>().materialsRoot; }
    ecs::Entity getAssetsRoot() const { return registry.ctx().get<SceneRoots>().assetsRoot; }
    ecs::Entity getObjectsRoot() const { return registry.ctx().get<SceneRoots>().objectsRoot; }
    ecs::Entity getDefaultMaterial() const { return defaultMaterial; }
    ecs::Entity getDefaultMesh() const { return defaultMesh; }
    ecs::Entity getDefaultCamera() const { return defaultCamera; }

    ecs::Entity& getCamera() { return activeCamera; }
    bool isPreviewing() { return activeCamera != defaultCamera; }   // TODO: find a better name
    void setActiveCamera(ecs::Entity newActiveCamera);
    bool resetActiveCamera();

    void bakePhysics();
    bool isPhysicsBakeInProgress() const;
    int getPhysicsBakeCurrentFrame() const;
    int getPhysicsBakeTotalFrames() const;

    void runPreRender() { preUpdateScheduler.run(registry); }
    void runOnRender(const FrameContext& frame) {
        registry.ctx().get<FrameContext>() = frame;
        onRenderScheduler.run(registry);
    }

    ecs::Registry& getRegistry() { return registry; }
    const ecs::Registry& getRegistry() const { return registry; }
    
    AnimationStore& getAnimationStore() { return animationStore; }

    SceneGpuBuffers& getBuffers() { return registry.ctx().get<SceneGpuBuffers>(); }
    const SceneGpuBuffers& getBuffers() const { return registry.ctx().get<SceneGpuBuffers>(); }
    void setGpuBufferHandles(SceneGpuBuffers handles);

    MeshAsset* getMeshAsset(ecs::Entity e);
    const MeshAsset* getMeshAsset(ecs::Entity e) const;

private:
    ecs::Registry registry;
    ecs::SystemScheduler preUpdateScheduler;
    ecs::SystemScheduler onRenderScheduler;

    ecs::Entity defaultMaterial;
    ecs::Entity defaultMesh;

    // TODO: find a better architecture place as this is an Editor object (not Core)
    // Used as the main viewport camera in the editor (until we preview)
    ecs::Entity defaultCamera;
    ecs::Entity activeCamera;

    AnimationStore animationStore;

    void initSystems();

    SceneRoots& roots() { return registry.ctx().get<SceneRoots>(); }

    void resetSceneState();
    void addDefaultAssets();

};
