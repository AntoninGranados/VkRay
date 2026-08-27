#include "scene.hpp"

#include <filesystem>
#include <format>
#include <optional>
#include <utility>

#include "core/core.hpp"
#include "core/ecs/components/camera.hpp"
#include "core/ecs/entity.hpp"
#include "utils/log.hpp"
#include "core/ecs/systems/gpu_packing_system.hpp"
#include "core/ecs/systems/aperture_system.hpp"
#include "core/ecs/systems/camera_system.hpp"
#include "core/ecs/systems/animation_system.hpp"
#include "core/ecs/systems/physics/physics_system.hpp"

void Scene::init() {
    registry.ctx().emplace<SceneRoots>();
    registry.ctx().emplace<SceneGpuBuffers>();
    registry.ctx().emplace<FrameContext>();
    registry.ctx().emplace<AnimationStore*>(&animationStore);
    registry.ctx().emplace<ecs::Entity*>(&activeCamera);
    initSystems();
    addDefaultAssets();
}

void Scene::destroy() {}

void Scene::setGpuBufferHandles(SceneGpuBuffers handles) {
    registry.ctx().get<SceneGpuBuffers>() = handles;
}

void Scene::clear() {
    SceneGpuBuffers& gpuBuffers = registry.ctx().get<SceneGpuBuffers>();
    gpuBuffers.sphere.capacity   = 0;
    gpuBuffers.plane.capacity    = 0;
    gpuBuffers.box.capacity      = 0;
    gpuBuffers.quad.capacity     = 0;
    gpuBuffers.vertex.capacity   = 0;
    gpuBuffers.index.capacity    = 0;
    gpuBuffers.bvh.capacity      = 0;
    gpuBuffers.mesh.capacity     = 0;
    gpuBuffers.material.capacity = 0;
    gpuBuffers.object.capacity   = 0;
    gpuBuffers.light.capacity    = 0;
    resetSceneState();
    addDefaultAssets();
}

const std::vector<ecs::Entity>& Scene::getChildren(ecs::Entity parent) const {
    return registry.getChildren(parent);
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

void Scene::setActiveCamera(ecs::Entity newActiveCamera) {
    if (newActiveCamera == activeCamera) return;
    activeCamera = newActiveCamera;
    Core::markDirty();
}

bool Scene::resetActiveCamera() {
    if (activeCamera == defaultCamera) return true;
    setActiveCamera(defaultCamera);
    return false;
}

void Scene::bakePhysics() {
    ecs::bakePhysicsSimulation(registry);
}

bool Scene::isPhysicsBakeInProgress() const {
    return ecs::isPhysicsBakeInProgress();
}

int Scene::getPhysicsBakeCurrentFrame() const {
    return ecs::getPhysicsBakeCurrentFrame();
}

int Scene::getPhysicsBakeTotalFrames() const {
    return ecs::getPhysicsBakeTotalFrames();
}

void Scene::initSystems() {
    preUpdateScheduler.clear();
    preUpdateScheduler.add(ecs::animationSystem);
    preUpdateScheduler.add(ecs::physicsSystem);
    preUpdateScheduler.add(ecs::apertureSystem);
    preUpdateScheduler.add(ecs::cameraPreUpdateSystem);

    onRenderScheduler.clear();
    onRenderScheduler.add(ecs::materialPackingSystem);
    onRenderScheduler.add(ecs::spherePackingSystem);
    onRenderScheduler.add(ecs::planePackingSystem);
    onRenderScheduler.add(ecs::boxPackingSystem);
    onRenderScheduler.add(ecs::quadPackingSystem);
    onRenderScheduler.add(ecs::meshPackingSystem);
    onRenderScheduler.add(ecs::objectPackingSystem);
    onRenderScheduler.add(ecs::lightPackingSystem);
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
    sceneRoots.materialsRoot = createNamedEntity("Materials");
    sceneRoots.assetsRoot    = createNamedEntity("Assets");
    sceneRoots.objectsRoot   = createNamedEntity("Objects");
    sceneRoots.internalsRoot = createNamedEntity("Internals");

    defaultMaterial = createNamedEntity("Default Material", sceneRoots.materialsRoot);
    registry.add(defaultMaterial, ecs::Diffuse);
    registry.get(defaultMaterial, ecs::Diffuse).set<glm::vec3>("albedo", glm::vec3(0.5f, 0.0f, 0.5f));

    defaultMesh = createNamedEntity("Default Cube", sceneRoots.assetsRoot);
    registry.add(defaultMesh, ecs::Mesh);
    registry.get(defaultMesh, ecs::Mesh).payload<MeshAsset>("geometry") = makeDefaultMeshAsset();

    defaultCamera = createNamedEntity("Default Camera", sceneRoots.internalsRoot);
    registry.add(defaultCamera, ecs::Camera);
    registry.get(defaultCamera, ecs::Camera).set<float>("fov", 80.0f);
    registry.get(defaultCamera, ecs::Transform).set<glm::vec3>("position", glm::vec3(0.0f, 0.0f, -10.0f));
    registry.get(defaultCamera, ecs::Transform).set<glm::vec3>("rotation", glm::vec3(0.0f, 180.0f, 0.0f));

    activeCamera = defaultCamera;
}

MeshAsset* Scene::getMeshAsset(ecs::Entity e) {
    return registry.has(e, ecs::Mesh) ? &registry.get(e, ecs::Mesh).payload<MeshAsset>("geometry") : nullptr;
}

const MeshAsset* Scene::getMeshAsset(ecs::Entity e) const {
    return registry.has(e, ecs::Mesh) ? &registry.get(e, ecs::Mesh).payload<MeshAsset>("geometry") : nullptr;
}

