#include "scene.hpp"

#include <filesystem>
#include <format>
#include <optional>
#include <utility>

#include "core/ecs/components/camera.hpp"
#include "core/ecs/components/core.hpp"
#include "core/ecs/entity.hpp"
#include "core/ecs/systems/aperture_system.hpp"
#include "core/ecs/systems/physics/physics_system.hpp"
#include "utils/log.hpp"

void Scene::initContext() {
    registry.ctx().emplace<SceneRoots>();
    registry.ctx().emplace<SceneGpuBuffers>();
    registry.ctx().emplace<MeshTemplates>();
    registry.ctx().emplace<ObjectIndices>();
    registry.ctx().emplace<CameraMotionInfo>();
    registry.ctx().emplace<CameraMediaInfo>();
    registry.ctx().emplace<LensPluginInfo>();
    registry.ctx().emplace<SkyPluginInfo>();
    registry.ctx().emplace<CompositingChainInfo>();
    registry.ctx().emplace<ecs::ApertureState>();
    registry.ctx().emplace<ecs::PhysicsBakeState>();
    registry.ctx().emplace<AnimationStore*>(&animationStore);
    registry.ctx().emplace<ecs::Entity*>(&activeCamera);
    addDefaultAssets();
}

void Scene::setGpuBufferHandles(SceneGpuBuffers handles) {
    registry.ctx().get<SceneGpuBuffers>() = handles;
    invalidateGpuData();
}

void Scene::clear() {
    resetSceneState();
    addDefaultAssets();
}

const std::vector<ecs::Entity>& Scene::getChildren(ecs::Entity parent) const { return registry.getChildren(parent); }

ecs::Entity Scene::getEnvironment() const { return findSceneRootChild(registry, ecs::Environment); }

ecs::Entity Scene::getCompositing() const { return findSceneRootChild(registry, ecs::Compositing); }

ecs::Entity Scene::findSceneRootChild(const ecs::Registry& registry, const ecs::ComponentType& type) {
    for (const ecs::Entity& e : registry.getChildren(registry.ctx().get<SceneRoots>().sceneRoot))
        if (registry.has(e, type)) return e;
    return {};
}

ecs::Entity Scene::loadMeshAsset(std::string name, const std::string& path, bool smooth) {
    std::optional<MeshAsset> asset = MeshAsset::load(path);
    if (!asset) return {};

    ecs::Entity e = createNamedEntity(std::move(name), roots().assetsRoot);
    registry.add(e, ecs::Mesh);
    ecs::Component& meshComponent = registry.get(e, ecs::Mesh);
    meshComponent.set<std::filesystem::path>("path", path);
    meshComponent.set<bool>("smooth", smooth);
    meshComponent.payload<MeshAsset>("geometry") = std::move(*asset);

    Log::success("Scene", std::format("Loaded mesh: {}", path));
    return e;
}

ecs::Entity Scene::createNamedEntity(std::string name, ecs::Entity parent) {
    ecs::Entity e = registry.createEntity(parent);
    registry.add(e, ecs::Name);
    registry.get(e, ecs::Name).set<std::string>("value", name);
    return e;
}

void Scene::resetSceneState() {
    registry.clear();
    animationStore.clear();
}

void Scene::addDefaultAssets() {
    SceneRoots& sceneRoots = roots();
    sceneRoots.sceneRoot = createNamedEntity("Scene");
    sceneRoots.materialsRoot = createNamedEntity("Materials");
    sceneRoots.assetsRoot = createNamedEntity("Assets");
    sceneRoots.objectsRoot = createNamedEntity("Objects");
    sceneRoots.internalsRoot = createNamedEntity("Internals");

    defaultMaterial = createNamedEntity("Default Material", sceneRoots.materialsRoot);
    registry.add(defaultMaterial, ecs::Diffuse);
    registry.get(defaultMaterial, ecs::Diffuse).set<glm::vec3>("albedo", glm::vec3(0.5f, 0.0f, 0.5f));

    defaultMesh = createNamedEntity("Default Cube", sceneRoots.assetsRoot);
    registry.add(defaultMesh, ecs::Mesh);
    registry.get(defaultMesh, ecs::Mesh).payload<MeshAsset>("geometry") = makeDefaultMeshAsset();

    const ecs::Entity environment = createNamedEntity("Environment", sceneRoots.sceneRoot);
    registry.add(environment, ecs::Environment);
    registry.add(environment, ecs::Physics);

    const ecs::Entity compositing = createNamedEntity("Compositing", sceneRoots.sceneRoot);
    registry.add(compositing, ecs::Compositing);

    defaultCamera = createNamedEntity("Default Camera", sceneRoots.internalsRoot);
    registry.add(defaultCamera, ecs::Camera);
    registry.get(defaultCamera, ecs::Transform).set<glm::vec3>("position", glm::vec3(0.0f, 0.0f, -10.0f));
    registry.get(defaultCamera, ecs::Transform).set<glm::vec3>("rotation", glm::vec3(0.0f, 180.0f, 0.0f));

    for (const ecs::Entity entity :
         {sceneRoots.sceneRoot, sceneRoots.materialsRoot, sceneRoots.assetsRoot, sceneRoots.objectsRoot,
          sceneRoots.internalsRoot, defaultMaterial, defaultMesh, environment, compositing, defaultCamera})
        registry.add(entity, ecs::Locked);

    activeCamera = defaultCamera;
}

MeshAsset* Scene::getMeshAsset(ecs::Entity e) {
    return registry.has(e, ecs::Mesh) ? &registry.get(e, ecs::Mesh).payload<MeshAsset>("geometry") : nullptr;
}

const MeshAsset* Scene::getMeshAsset(ecs::Entity e) const {
    return registry.has(e, ecs::Mesh) ? &registry.get(e, ecs::Mesh).payload<MeshAsset>("geometry") : nullptr;
}
