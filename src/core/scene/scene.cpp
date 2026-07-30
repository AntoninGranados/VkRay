#include "scene.hpp"

#include <cassert>
#include <format>
#include <utility>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

#include "utils/log.hpp"
#include "core/scene/object/material.hpp"
#include "core/ecs/systems/gpu_packing_system.hpp"
#include "core/ecs/systems/camera_system.hpp"
#include "core/ecs/systems/animation_system.hpp"
#include "core/ecs/systems/physics/physics_system.hpp"

// Public
void Scene::init() {
    initSystems();
    ensureDefaultAssets();
}

void Scene::destroy() {}

void Scene::setGpuBufferHandles(SceneGpuBuffers handles) {
    gpuBuffers = handles;
}

void Scene::clear() {
    // Reset capacity so the next frame resizes buffers down to their initial size
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
    ensureDefaultAssets();
}

MaterialHandle Scene::pushMaterial(const Material& mat) {
    const MaterialHandle materialHandle = static_cast<int>(materials.size());
    materials.push_back(mat);
    return materialHandle;
}

void Scene::pushSphere(std::string name, glm::vec3 center, float radius, MaterialHandle materialHandle) {
    ecs::Entity e = createNamedEntity(std::move(name));

    ecs::Component& sphere = registry.add(e, ecs::Sphere);
    sphere.set<float>("radius", radius);

    addMaterialRef(e, materialHandle);

    ecs::Component& t = registry.add(e, ecs::Transform);
    t.set<glm::vec3>("position", center);
    t.set<glm::vec3>("scale", glm::vec3(1.0f));

    entities.push_back(e);
}

void Scene::pushPlane(std::string name, glm::vec3 point, glm::vec3 normal, MaterialHandle materialHandle) {
    ecs::Entity e = createNamedEntity(std::move(name));

    registry.add(e, ecs::Plane);

    addMaterialRef(e, materialHandle);

    const glm::quat q = glm::rotation(glm::vec3(0.0f, 1.0f, 0.0f), normal);
    ecs::Component& t = registry.add(e, ecs::Transform);
    t.set<glm::vec3>("position", point);
    t.set<glm::vec3>("rotation", glm::degrees(glm::eulerAngles(q)));
    t.set<glm::vec3>("scale", glm::vec3(1.0f));

    entities.push_back(e);
}

void Scene::pushBox(std::string name, glm::vec3 cornerMin, glm::vec3 cornerMax, MaterialHandle materialHandle) {
    glm::vec3 center = (cornerMin + cornerMax) * 0.5f;
    glm::vec3 halfExtents = (cornerMax - cornerMin) * 0.5f;
    ecs::Entity e = createNamedEntity(std::move(name));

    registry.add(e, ecs::Box);

    addMaterialRef(e, materialHandle);

    ecs::Component& t = registry.add(e, ecs::Transform);
    t.set<glm::vec3>("position", center);
    t.set<glm::vec3>("scale", halfExtents);

    entities.push_back(e);
}

void Scene::pushQuad(std::string name, glm::vec3 center, glm::vec3 normal, glm::vec2 scale, float rotation, MaterialHandle materialHandle) {
    ecs::Entity e = createNamedEntity(std::move(name));

    normal = glm::normalize(normal);
    glm::vec3 ref = std::abs(glm::dot(normal, glm::vec3(0.0f, 1.0f, 0.0f))) < 0.99f
        ? glm::vec3(0.0f, 1.0f, 0.0f)
        : glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 tangent   = glm::normalize(glm::cross(ref, normal));
    glm::vec3 bitangent = glm::normalize(glm::cross(normal, tangent));

    const glm::vec3 u_hat = std::cos(rotation) * tangent + std::sin(rotation) * bitangent;
    const glm::vec3 v_hat = -std::sin(rotation) * tangent + std::cos(rotation) * bitangent;

    registry.add(e, ecs::Quad);

    addMaterialRef(e, materialHandle);

    const glm::quat q = glm::quat_cast(glm::mat3(u_hat, v_hat, normal));
    ecs::Component& t = registry.add(e, ecs::Transform);
    t.set<glm::vec3>("position", center);
    t.set<glm::vec3>("rotation", glm::degrees(glm::eulerAngles(q)));
    t.set<glm::vec3>("scale", glm::vec3(scale.x, scale.y, 1.0f));

    entities.push_back(e);
}

void Scene::pushMesh(std::string name, const std::string& path, const glm::mat4& transform, MaterialHandle materialHandle, bool smoothShading) {
    MeshHandle handle = static_cast<MeshHandle>(meshAssets.size());
    meshAssets.emplace_back(MeshAsset(name));
    MeshAsset& asset = meshAssets.back();
    asset.setSmoothShading(smoothShading);
    if (!asset.loadFromObj(path)) {
        meshAssets.pop_back();
        return;
    }

    Log::success("Scene", std::format("Loaded mesh: {}", name));
    pushMesh(name, handle, transform, materialHandle);
}

void Scene::pushMesh(std::string name, MeshHandle meshHandle, const glm::mat4& transform, MaterialHandle materialHandle) {
    ecs::Entity e = createNamedEntity(std::move(name));

    ecs::Component& mesh = registry.add(e, ecs::MeshRef);
    mesh.set<int>("handle", meshHandle);

    addMaterialRef(e, materialHandle);
    addTransformFromMatrix(e, transform);

    entities.push_back(e);
}

void Scene::pushCamera(std::string name, const glm::mat4& transform) {
    ecs::Entity e = createNamedEntity(std::move(name));
    
    ecs::Component& cam = registry.add(e, ecs::Camera);
    cam.set<float>("fov", 60.0f);
    cam.set<float>("aperture", 0.0f);
    cam.set<float>("focus_depth", 1.0f);

    addTransformFromMatrix(e, transform);

    entities.push_back(e);
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

bool Scene::checkUpdate() {
    if (updated) {
        updated = false;
        return true;
    }
    return false;
}

// Private helpers
void Scene::initSystems() {
    preUpdateScheduler.clear();
    preUpdateScheduler.add(ecs::animationSystem);
    preUpdateScheduler.add(ecs::physicsSystem);
    preUpdateScheduler.add(ecs::cameraPreUpdateSystem);

    onRenderScheduler.clear();
    onRenderScheduler.add(ecs::spherePackingSystem);
    onRenderScheduler.add(ecs::planePackingSystem);
    onRenderScheduler.add(ecs::boxPackingSystem);
    onRenderScheduler.add(ecs::quadPackingSystem);
    onRenderScheduler.add(ecs::meshPackingSystem);
    onRenderScheduler.add(ecs::objectPackingSystem);
    onRenderScheduler.add(ecs::materialPackingSystem);
    onRenderScheduler.add(ecs::lightPackingSystem);

    postUpdateScheduler.clear();
    postUpdateScheduler.add(ecs::cameraPostUpdateSystem);
}


ecs::Entity Scene::createNamedEntity(std::string name) {
    ecs::Entity e = registry.createEntity();
    ecs::Component& nameComp = registry.add(e, ecs::Name);
    nameComp.set<std::string>("value", name);
    return e;
}

void Scene::addMaterialRef(ecs::Entity e, MaterialHandle materialHandle) {
    ecs::Component& mat = registry.add(e, ecs::MaterialRef);
    mat.set<int>("handle", materialHandle);
}

void Scene::addTransformFromMatrix(ecs::Entity e, const glm::mat4& transform) {
    glm::vec3 translation, scale, skew;
    glm::vec4 perspective;
    glm::quat rotation;
    glm::decompose(transform, scale, rotation, translation, skew, perspective);
    ecs::Component& t = registry.add(e, ecs::Transform);
    t.set<glm::vec3>("position", translation);
    t.set<glm::vec3>("rotation", glm::degrees(glm::eulerAngles(glm::normalize(rotation))));
    t.set<glm::vec3>("scale", scale);
}

void Scene::resetSceneState() {
    for (auto& e : entities) {
        registry.destroyEntity(e);
    }

    entities.clear();
    materials.clear();
    meshAssets.clear();
    animationStore.clear();
}

void Scene::ensureDefaultAssets() {
    pushMaterial(DEFAULT_MATERIAL);
    meshAssets.push_back(makeDefaultMeshAsset());
}
