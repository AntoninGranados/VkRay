#include "scene.hpp"

#include <cassert>
#include <format>
#include <utility>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "imgui/ImGuizmo.h"

#include "app/log.hpp"
#include "scene/object/material.hpp"
#include "scene_editor_ui.hpp"

#include "ecs/systems/transform_system.hpp"
#include "ecs/systems/gpu_packing_system.hpp"
#include "ecs/systems/camera_system.hpp"
#include "ecs/systems/animation_system.hpp"
#include "ecs/systems/physics/physics_system.hpp"

// Public
void Scene::init() {
    assert(ctx && ctx->engine);

    auto& uiReg = ecs::ComponentUiRegistry::get();
    uiReg.setMaterials(&materials);
    uiReg.setMeshAssets(&meshAssets);
    ecs::ComponentUiRegistry::init();

    initSystems();
    ensureDefaultAssets();
}

void Scene::destroy() {}

void Scene::setGpuBufferHandles(SceneGpuBuffers handles) {
    gpuBuffers = handles;
}

void Scene::clear() {
    assert(ctx && ctx->engine);
    ctx->engine->waitIdle();

    // Reset capacity so the next frame resizes buffers down to their initial size
    gpuBuffers.sphere.capacity   = 0;
    gpuBuffers.plane.capacity    = 0;
    gpuBuffers.box.capacity      = 0;
    gpuBuffers.quad.capacity      = 0;
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

    ecs::Sphere sphereComponent;
    sphereComponent.setRadius(radius);
    registry.add<ecs::Sphere>(e, sphereComponent);

    addMaterialRef(e, materialHandle);

    ecs::Transform transformComponent;
    transformComponent.setPosition(center);
    registry.add<ecs::Transform>(e, transformComponent);

    entities.push_back(e);
}

void Scene::pushPlane(std::string name, glm::vec3 point, glm::vec3 normal, MaterialHandle materialHandle) {
    ecs::Entity e = createNamedEntity(std::move(name));

    registry.add<ecs::Plane>(e, ecs::Plane{});

    addMaterialRef(e, materialHandle);
    
    ecs::Transform transformComponent;
    transformComponent.setPosition(point);
    transformComponent.setRotation(glm::rotation(glm::vec3(0.0f, 1.0f, 0.0f), normal));
    registry.add<ecs::Transform>(e, transformComponent);

    entities.push_back(e);
}

void Scene::pushBox(std::string name, glm::vec3 cornerMin, glm::vec3 cornerMax, MaterialHandle materialHandle) {
    glm::vec3 center = (cornerMin + cornerMax) * 0.5f;
    glm::vec3 halfExtents = (cornerMax - cornerMin) * 0.5f;
    ecs::Entity e = createNamedEntity(std::move(name));

    registry.add<ecs::Box>(e, ecs::Box{});

    addMaterialRef(e, materialHandle);
    
    ecs::Transform transformComponent;
    transformComponent.setPosition(center);
    transformComponent.setScale(halfExtents);
    registry.add<ecs::Transform>(e, transformComponent);

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

    glm::vec3 u = (std::cos(rotation) * tangent + std::sin(rotation) * bitangent) * scale.x;
    glm::vec3 v = (-std::sin(rotation) * tangent + std::cos(rotation) * bitangent) * scale.y;

    registry.add<ecs::Quad>(e, ecs::Quad{ .u = u, .v = v, .normal = normal });

    addMaterialRef(e, materialHandle);

    ecs::Transform transformComponent;
    transformComponent.setPosition(center - 0.5f * (u + v));
    registry.add<ecs::Transform>(e, transformComponent);

    entities.push_back(e);
}

void Scene::pushMesh(std::string name, const std::string& path, const glm::mat4& transform, MaterialHandle materialHandle, bool smoothShading) {
    MeshHandle handle = static_cast<MeshHandle>(meshAssets.size());
    meshAssets.emplace_back(MeshAsset(name));
    MeshAsset& asset = meshAssets.back();
    asset.setSmoothShading(smoothShading);
    if (!asset.loadFromObj(*ctx, path)) {
        meshAssets.pop_back();
        return;
    }

    Log::success("Scene", std::format("Loaded mesh: {}", name));
    pushMesh(name, handle, transform, materialHandle);
}

void Scene::pushMesh(std::string name, MeshHandle meshHandle, const glm::mat4& transform, MaterialHandle materialHandle) {
    ecs::Entity e = createNamedEntity(std::move(name));

    ecs::MeshRef meshRef;
    meshRef.setHandle(meshHandle);
    registry.add<ecs::MeshRef>(e, meshRef);

    addMaterialRef(e, materialHandle);
    registry.add<ecs::Transform>(e, makeTransformFromMatrix(transform));

    entities.push_back(e);
}

void Scene::pushCamera(std::string name, const glm::mat4& transform) {
    ecs::Entity e = createNamedEntity(std::move(name));
    
    ecs::CameraObject cameraComponent;
    cameraComponent.setFov(60.0f);
    cameraComponent.setAperture(0.0f);
    cameraComponent.setFocusDepth(1.0f);
    registry.add<ecs::CameraObject>(e, cameraComponent);

    registry.add<ecs::Transform>(e, makeTransformFromMatrix(transform));

    entities.push_back(e);
}


void Scene::drawGuizmo(const glm::mat4& view, const glm::mat4& proj) { SceneEditorUI::drawGuizmo(*this, view, proj); }

void Scene::drawUI() { SceneEditorUI::drawUI(*this); }

void Scene::drawNewObjectPopUp() { SceneEditorUI::drawNewObjectPopUp(*this); }

void Scene::drawSelectedEntityUI() { SceneEditorUI::drawSelectedEntityUI(*this); }

void Scene::drawSelectedMaterialUI() { SceneEditorUI::drawSelectedMaterialUI(*this); }

void Scene::drawSelectedMeshAssetUI() { SceneEditorUI::drawSelectedMeshAssetUI(*this); }

void Scene::bakePhysics() {
    ecs::bakePhysicsSimulation(registry, *ctx);
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


bool Scene::raycast(const glm::vec2& screenPos, const glm::vec2& screenSize, float& dist, glm::vec3& p, bool select, bool includeCameras) {
    const Ray ray = getRay(screenPos, screenSize, camera);
    float tClosest = std::numeric_limits<float>::infinity();
    int idClosest = -1;

    float t = -1.0f;

    auto& sphereStorage = registry.storage<ecs::Sphere>();
    auto& planeStorage = registry.storage<ecs::Plane>();
    auto& boxStorage = registry.storage<ecs::Box>();
    auto& quadStorage = registry.storage<ecs::Quad>();
    auto& meshStorage = registry.storage<ecs::MeshRef>();
    auto& cameraStorage = registry.storage<ecs::CameraObject>();
    auto& transformStorage = registry.storage<ecs::Transform>();

    for (size_t i = 0; i < entities.size(); i++) {
        const ecs::Entity& e = entities[i];

        if (!transformStorage.has(e)) continue;

        auto& transform = transformStorage.get(e);
        
        if (sphereStorage.has(e)) {
            auto& sphere = sphereStorage.get(e);
            t = raySphereIntersection(ray, transform.position, sphere.radius);
        } else if (planeStorage.has(e)) {
            glm::vec3 normal = glm::normalize(transform.rotation * glm::vec3(0.0f, 1.0f, 0.0f));
            t = rayPlaneIntersection(ray, transform.position, normal);
        } else if (boxStorage.has(e)) {
            t = rayBoxIntersection(ray, transform.local);
        } else if (quadStorage.has(e)) {
            const ecs::Quad& q = quadStorage.get(e);
            t = rayQuadIntersection(ray, transform.position, q.u, q.v, q.normal);
        } else if (meshStorage.has(e)) {
            const ecs::MeshRef& meshRef = meshStorage.get(e);
            if (meshRef.handle >= 0 && static_cast<size_t>(meshRef.handle) < meshAssets.size()) {
                const MeshAsset& asset = meshAssets[meshRef.handle];
                t = rayMeshIntersection(ray, transform.local, asset.getVertices(), asset.getIndices());
            }
        } else if (includeCameras && cameraStorage.has(e)) {
            if (cameraStorage.get(e).isPreview) continue;
            constexpr float cameraSelectRadius = 0.6f;
            t = raySphereIntersection(ray, transform.position, cameraSelectRadius);
        }

        if (t >= 0.0f && t < tClosest) {
            tClosest = t;
            idClosest = i;
        }
    }
    
    if (select) selectedEntity = idClosest;
    dist = tClosest;
    p = ray.origin + dist * ray.dir;
    return idClosest >= 0;
}

const ecs::Entity* Scene::getSelectedEntity() const {
    if (selectedEntity < 0 || static_cast<size_t>(selectedEntity) >= entities.size())
        return nullptr;
    return &entities[static_cast<size_t>(selectedEntity)];
}

bool Scene::isPreviewingCamera() {
    if (ctx->renderMode != RenderMode::Preview) return false; // can't preview in render mode (a CameraObject is used but is should not be considered as a preview camera)

    auto& cameras = registry.storage<ecs::CameraObject>();
    for (const auto& e : cameras.entities()) {
        if (cameras.get(e).isPreview)
            return true;
    }
    return false;
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
    preUpdateScheduler.add(ecs::transformAnimationSystem);
    preUpdateScheduler.add(ecs::materialAnimationSystem);
    preUpdateScheduler.add(ecs::physicsSystem);
    preUpdateScheduler.add(ecs::transformSystem);
    preUpdateScheduler.add(ecs::cameraPreUpdateSystem);

    onRenderScheduler.clear();
    onRenderScheduler.add(ecs::spherePackingSystem);
    onRenderScheduler.add(ecs::planePackingSystem);
    onRenderScheduler.add(ecs::boxPackingSystem);
    onRenderScheduler.add(ecs::quadPackingSystem);
    onRenderScheduler.add(ecs::meshPackingSystem);
    onRenderScheduler.add(ecs::materialPackingSystem);
    onRenderScheduler.add(ecs::objectPackingSystem);
    onRenderScheduler.add(ecs::lightPackingSystem);

    onUiScheduler.clear();
    onUiScheduler.add(ecs::cameraDrawingSystem);

    postUpdateScheduler.clear();
    postUpdateScheduler.add(ecs::cameraPostUpdateSystem);
}


ecs::Entity Scene::createNamedEntity(std::string name) {
    ecs::Entity e = registry.createEntity();
    ecs::Name nameComponent;
    nameComponent.setValue(std::move(name));
    registry.add<ecs::Name>(e, nameComponent);
    return e;
}

void Scene::addMaterialRef(ecs::Entity e, MaterialHandle materialHandle) {
    ecs::MaterialRef materialRef;
    materialRef.setHandle(materialHandle);
    registry.add<ecs::MaterialRef>(e, materialRef);
}

ecs::Transform Scene::makeTransformFromMatrix(const glm::mat4& transform) const {
    glm::vec3 translation, rotationEuler, scale;
    ImGuizmo::DecomposeMatrixToComponents(
        glm::value_ptr(transform),
        glm::value_ptr(translation),
        glm::value_ptr(rotationEuler),
        glm::value_ptr(scale));

    ecs::Transform out;
    out.setPosition(translation);
    out.setRotation(glm::quat(glm::radians(rotationEuler)));
    out.setScale(scale);
    return out;
}

void Scene::resetSceneState() {
    for (auto& e : entities) {
        registry.destroyEntity(e);
    }

    entities.clear();
    entityN = 0;
    materials.clear();
    materialN = 0;
    meshAssets.clear();
    selectedEntity = -1;
    selectedMaterial = -1;
    selectedMeshAsset = -1;
}

void Scene::ensureDefaultAssets() {
    pushMaterial(DEFAULT_MATERIAL);
    meshAssets.push_back(makeDefaultMeshAsset());
}
