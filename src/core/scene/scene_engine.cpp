#include "scene.hpp"

#include "core/core.hpp"
#include "core/ecs/components/camera.hpp"
#include "core/ecs/systems/animation_system.hpp"
#include "core/ecs/systems/aperture_system.hpp"
#include "core/ecs/systems/camera_focus_system.hpp"
#include "core/ecs/systems/gpu_packing_system.hpp"
#include "core/ecs/systems/physics/physics_system.hpp"

void Scene::init() {
    initContext();
    initSystems();
}

void Scene::destroy() {}

void Scene::setActiveCamera(ecs::Entity newActiveCamera) {
    if (newActiveCamera == activeCamera) return;
    activeCamera = newActiveCamera;
    Core::markRenderDirty();
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

    onRenderScheduler.clear();
    onRenderScheduler.add(ecs::materialPackingSystem);
    onRenderScheduler.add(ecs::meshPackingSystem);
    onRenderScheduler.add(ecs::objectPackingSystem);
    onRenderScheduler.add(ecs::lightPackingSystem);
}
