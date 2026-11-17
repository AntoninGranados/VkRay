#pragma once

#include <array>
#include <optional>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "core/animation/animation_store.hpp"
#include "core/camera/camera.hpp"
#include "core/ecs/components.hpp"
#include "core/ecs/entity.hpp"
#include "core/ecs/registry.hpp"
#include "core/ecs/system_scheduler.hpp"
#include "core/ecs/systems/aperture_system.hpp"
#include "core/ecs/systems/physics/physics_system.hpp"
#include "core/render_structures.hpp"
#include "core/scene/asset/mesh.hpp"
#include "core/scene/gpu_structs.hpp"

struct SceneGpuBuffers;

struct CameraMediaInfo {
    int count = 0;
    std::array<int32_t, kMediumStackSize> objects{};
};

struct LensPluginInfo {
    int32_t slot = -1;
    int32_t paramsBase = 0;
};

struct SkyPluginInfo {
    int32_t slot = 0;
    int32_t paramsBase = 0;
};

struct CompositingPassInfo {
    int32_t slot = 0;
    int32_t paramsBase = 0;
    int32_t passId = 0;
};

struct PackingInfo {
    std::vector<GpuMesh> meshTemplates;
    std::vector<GpuMesh> meshInstances;
    std::vector<GpuObject> objects;
    std::unordered_map<ecs::Entity, int> objectIndices;
    uint32_t cameraMotionOffset = 0;
    CameraMediaInfo cameraMedia;
    LensPluginInfo lens;
    SkyPluginInfo sky;
    std::vector<CompositingPassInfo> compositingPasses;
    TextureSlots textureSlots;
    uint32_t textureGeneration = 0;
};

struct SceneRoots {
    ecs::Entity materialsRoot;
    ecs::Entity assetsRoot;
    ecs::Entity objectsRoot;
    ecs::Entity sceneRoot;
    ecs::Entity internalsRoot;
};

class Scene {
public:
    Scene() { addDefaultAssets(); }
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    void init(SceneGpuBuffers& buffers);
    void clear();

    ecs::Entity createNamedEntity(std::string name, ecs::Entity parent = {});
    ecs::Entity loadMeshAsset(std::string name, const std::string& path, bool smooth = false);

    const std::vector<ecs::Entity>& getChildren(ecs::Entity parent) const;

    ecs::Entity getMaterialsRoot() const { return roots.materialsRoot; }
    ecs::Entity getAssetsRoot() const { return roots.assetsRoot; }
    ecs::Entity getObjectsRoot() const { return roots.objectsRoot; }
    ecs::Entity getSceneRoot() const { return roots.sceneRoot; }
    ecs::Entity getInternalsRoot() const { return roots.internalsRoot; }
    ecs::Entity getDefaultMaterial() const { return defaultMaterial; }
    ecs::Entity getDefaultMesh() const { return defaultMesh; }
    ecs::Entity getEnvironment() const;
    ecs::Entity getCompositing() const;

    std::optional<ecs::Entity> getActiveCamera() const { return activeCamera; }
    void setActiveCamera(ecs::Entity newActiveCamera);

    void runPreRender() { preUpdateScheduler.run(registry); }
    void runOnRender() { onRenderScheduler.run(registry); }

    ecs::Registry& getRegistry() { return registry; }
    const ecs::Registry& getRegistry() const { return registry; }

    AnimationStore& getAnimationStore() { return animationStore; }
    PackingInfo& getPackingInfo() { return packingInfo; }
    const PackingInfo& getPackingInfo() const { return packingInfo; }
    ecs::PhysicsBakeState& getPhysicsBakeState() { return physicsBakeState; }
    const ecs::PhysicsBakeState& getPhysicsBakeState() const { return physicsBakeState; }

    MeshAsset* getMeshAsset(ecs::Entity e);
    const MeshAsset* getMeshAsset(ecs::Entity e) const;

private:
    ecs::Registry registry;
    ecs::SystemScheduler preUpdateScheduler;
    ecs::SystemScheduler onRenderScheduler;

    SceneRoots roots;
    ecs::Entity defaultMaterial;
    ecs::Entity defaultMesh;

    std::optional<ecs::Entity> activeCamera;

    AnimationStore animationStore;
    PackingInfo packingInfo;
    ecs::ApertureState apertureState;
    ecs::PhysicsBakeState physicsBakeState;

    ecs::Entity findSceneRootChild(const ecs::ComponentType& type) const;

    void resetSceneState();
    void addDefaultAssets();
};
