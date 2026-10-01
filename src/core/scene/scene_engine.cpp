#include "scene.hpp"

#include <initializer_list>
#include <vector>

#include "core/ecs/components/camera.hpp"
#include "core/ecs/systems/animation_system.hpp"
#include "core/ecs/systems/aperture_system.hpp"
#include "core/ecs/systems/camera_focus_system.hpp"
#include "core/ecs/systems/gpu_packing_system.hpp"
#include "core/ecs/systems/motion_sampling.hpp"
#include "core/ecs/systems/physics/physics_system.hpp"

void Scene::init() {
    initContext();
    initSystems();
}

void Scene::destroy() {}

void Scene::setActiveCamera(ecs::Entity newActiveCamera) {
    if (newActiveCamera == activeCamera) return;
    activeCamera = newActiveCamera;
    registry.markChanged(ecs::Camera);
}

bool Scene::resetActiveCamera() {
    if (activeCamera == defaultCamera) return true;
    setActiveCamera(defaultCamera);
    return false;
}

void Scene::activateSceneCamera() {
    for (const ecs::Entity& entity : registry.storage(ecs::Camera).entities()) {
        if (entity == defaultCamera) continue;
        setActiveCamera(entity);
        return;
    }
    setActiveCamera(defaultCamera);
}

void Scene::bakePhysics() { ecs::bakePhysicsSimulation(registry); }

bool Scene::isPhysicsBakeInProgress() const { return ecs::isPhysicsBakeInProgress(registry); }

int Scene::getPhysicsBakeCurrentFrame() const { return ecs::getPhysicsBakeCurrentFrame(registry); }

int Scene::getPhysicsBakeTotalFrames() const { return ecs::getPhysicsBakeTotalFrames(registry); }

void Scene::initSystems() {
    preUpdateScheduler.clear();
    preUpdateScheduler.add(ecs::animationSystem);
    preUpdateScheduler.add(ecs::physicsSystem);
    preUpdateScheduler.add(ecs::apertureSystem);
    preUpdateScheduler.add(ecs::cameraFocusSystem);

    using Types = std::vector<const ecs::ComponentType*>;
    const auto join = [](std::initializer_list<Types> lists) {
        Types joined;
        for (const Types& list : lists) joined.insert(joined.end(), list.begin(), list.end());
        return joined;
    };
    const Types materials = ecs::ComponentType::inGroup("material");
    const Types objects = ecs::ComponentType::inGroup("object");
    const Types cameras = ecs::ComponentType::inGroup("camera");
    const Types environments = ecs::ComponentType::inGroup("environment");

    onRenderScheduler.clear();
    onRenderScheduler.add(ecs::materialPackingSystem, join({materials, cameras, environments, {&ecs::Compositing}}));
    onRenderScheduler.add(ecs::meshPackingSystem, {&ecs::Mesh});
    onRenderScheduler.add(ecs::meshInstancePackingSystem, {&ecs::Mesh, &ecs::MeshRef, &ecs::Transform});
    onRenderScheduler.add(ecs::objectPackingSystem,
                          join({objects, materials, {&ecs::Transform, &ecs::Camera, &ecs::RigidBody}}),
                          ecs::isMotionBlurActive);
    onRenderScheduler.add(ecs::lightPackingSystem, join({objects, materials, {&ecs::Transform, &ecs::Mesh}}));
}
