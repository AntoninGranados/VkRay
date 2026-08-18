#include "scene.hpp"

#include <filesystem>
#include <format>
#include <optional>
#include <utility>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

#include "utils/log.hpp"
#include "core/ecs/systems/gpu_packing_system.hpp"
#include "core/ecs/systems/aperture_system.hpp"
#include "core/ecs/systems/camera_system.hpp"
#include "core/ecs/systems/animation_system.hpp"
#include "core/ecs/systems/physics/physics_system.hpp"

void Scene::init() {
    initSystems();
    addDefaultAssets();
}

void Scene::destroy() {}

void Scene::setGpuBufferHandles(SceneGpuBuffers handles) {
    gpuBuffers = handles;
}

void Scene::clear() {
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

ecs::Entity Scene::pushMaterial(const ecs::ComponentType& bsdfType, std::string name) {
    ecs::Entity e = createNamedEntity(std::move(name), materialsRoot);
    registry.add(e, bsdfType);
    return e;
}

void Scene::pushSphere(std::string name, glm::vec3 center, float radius, std::optional<ecs::Entity> materialEntity) {
    ecs::Entity e = createNamedEntity(std::move(name), objectsRoot);

    registry.add(e, ecs::Sphere);
    registry.get(e, ecs::Sphere).set<float>("radius", radius);

    pushMaterialRef(e, materialEntity);
    setTransform(e, center, glm::vec3(0.0f), glm::vec3(1.0f));
}

void Scene::pushPlane(std::string name, glm::vec3 point, glm::vec3 normal, std::optional<ecs::Entity> materialEntity) {
    ecs::Entity e = createNamedEntity(std::move(name), objectsRoot);

    registry.add(e, ecs::Plane);
    pushMaterialRef(e, materialEntity);

    const glm::quat q = glm::rotation(glm::vec3(0.0f, 1.0f, 0.0f), normal);
    setTransform(e, point, glm::degrees(glm::eulerAngles(q)), glm::vec3(1.0f));
}

void Scene::pushBox(std::string name, glm::vec3 cornerMin, glm::vec3 cornerMax, std::optional<ecs::Entity> materialEntity) {
    glm::vec3 center = (cornerMin + cornerMax) * 0.5f;
    glm::vec3 halfExtents = (cornerMax - cornerMin) * 0.5f;
    ecs::Entity e = createNamedEntity(std::move(name), objectsRoot);

    registry.add(e, ecs::Box);
    pushMaterialRef(e, materialEntity);
    setTransform(e, center, glm::vec3(0.0f), halfExtents);
}

void Scene::pushQuad(std::string name, glm::vec3 center, glm::vec3 normal, glm::vec2 scale, float rotation, std::optional<ecs::Entity> materialEntity) {
    ecs::Entity e = createNamedEntity(std::move(name), objectsRoot);

    normal = glm::normalize(normal);
    glm::vec3 ref = std::abs(glm::dot(normal, glm::vec3(0.0f, 1.0f, 0.0f))) < 0.99f
        ? glm::vec3(0.0f, 1.0f, 0.0f)
        : glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 tangent   = glm::normalize(glm::cross(ref, normal));
    glm::vec3 bitangent = glm::normalize(glm::cross(normal, tangent));

    const glm::vec3 u_hat = std::cos(rotation) * tangent + std::sin(rotation) * bitangent;
    const glm::vec3 v_hat = -std::sin(rotation) * tangent + std::cos(rotation) * bitangent;

    registry.add(e, ecs::Quad);
    pushMaterialRef(e, materialEntity);

    const glm::quat q = glm::quat_cast(glm::mat3(u_hat, v_hat, normal));
    setTransform(e, center, glm::degrees(glm::eulerAngles(q)), glm::vec3(scale.x, scale.y, 1.0f));
}

ecs::Entity Scene::pushMeshAsset(std::string name, const std::string& path, bool smooth) {
    std::optional<MeshAsset> asset = MeshAsset::load(path);
    if (!asset) return {};

    ecs::Entity e = createNamedEntity(std::move(name), assetsRoot);
    registry.add(e, ecs::Mesh);
    ecs::Component& meshComponent = registry.get(e, ecs::Mesh);
    meshComponent.set<std::filesystem::path>("path", path);
    meshComponent.set<bool>("smooth", smooth);
    meshComponent.payload<MeshAsset>("geometry") = std::move(*asset);

    Log::success("Scene", std::format("Loaded mesh: {}", path));
    return e;
}

void Scene::pushMesh(std::string name, const std::string& path, const glm::mat4& transform, std::optional<ecs::Entity> materialEntity, bool smooth) {
    ecs::Entity meshAssetEntity = pushMeshAsset(std::filesystem::path(path).stem().string(), path, smooth);
    if (meshAssetEntity == ecs::Entity{}) return;
    pushMesh(std::move(name), meshAssetEntity, transform, materialEntity);
}

void Scene::pushMesh(std::string name, ecs::Entity meshAssetEntity, const glm::mat4& transform, std::optional<ecs::Entity> materialEntity) {
    ecs::Entity e = createNamedEntity(std::move(name), objectsRoot);

    registry.add(e, ecs::MeshRef);
    registry.get(e, ecs::MeshRef).set<ecs::Entity>("handle", meshAssetEntity);

    pushMaterialRef(e, materialEntity);
    addTransformFromMatrix(e, transform);
}

void Scene::pushCamera(std::string name, const glm::mat4& transform) {
    ecs::Entity e = createNamedEntity(std::move(name), objectsRoot);

    registry.add(e, ecs::Camera);
    registry.get(e, ecs::Camera).set<float>("fov", 60.0f);

    addTransformFromMatrix(e, transform);
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

void Scene::pushMaterialRef(ecs::Entity e, std::optional<ecs::Entity> materialEntity) {
    registry.add(e, ecs::MaterialRef);
    registry.get(e, ecs::MaterialRef).set<ecs::Entity>("handle", materialEntity.value_or(defaultMaterial));
}

void Scene::setTransform(ecs::Entity e, glm::vec3 position, glm::vec3 rotation, glm::vec3 scale) {
    registry.add(e, ecs::Transform);
    auto& t = registry.get(e, ecs::Transform);
    t.set<glm::vec3>("position", position);
    t.set<glm::vec3>("rotation", rotation);
    t.set<glm::vec3>("scale", scale);
}

void Scene::addTransformFromMatrix(ecs::Entity e, const glm::mat4& transform) {
    glm::vec3 translation, scale, skew;
    glm::vec4 perspective;
    glm::quat rotation;
    glm::decompose(transform, scale, rotation, translation, skew, perspective);
    setTransform(e, translation, glm::degrees(glm::eulerAngles(glm::normalize(rotation))), scale);
}

void Scene::resetSceneState() {
    registry.clear();
    animationStore.clear();
    camera.clearPreviewCamera();
}

void Scene::addDefaultAssets() {
    materialsRoot = createNamedEntity("Materials");
    assetsRoot    = createNamedEntity("Assets");
    objectsRoot   = createNamedEntity("Objects");

    defaultMaterial = pushMaterial(ecs::Diffuse, "Default");
    registry.get(defaultMaterial, ecs::Diffuse).set<glm::vec3>("albedo", glm::vec3(1.0f, 0.0f, 1.0f));

    defaultMeshAsset = createNamedEntity("Default", assetsRoot);
    registry.add(defaultMeshAsset, ecs::Mesh);
    registry.get(defaultMeshAsset, ecs::Mesh).payload<MeshAsset>("geometry") = makeDefaultMeshAsset();
}

MeshAsset* Scene::getMeshAsset(ecs::Entity e) {
    return registry.has(e, ecs::Mesh) ? &registry.get(e, ecs::Mesh).payload<MeshAsset>("geometry") : nullptr;
}

const MeshAsset* Scene::getMeshAsset(ecs::Entity e) const {
    return registry.has(e, ecs::Mesh) ? &registry.get(e, ecs::Mesh).payload<MeshAsset>("geometry") : nullptr;
}
