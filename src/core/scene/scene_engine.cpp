#include "scene.hpp"

#include "core/ecs/components/camera.hpp"
#include "core/ecs/systems/animation_system.hpp"
#include "core/ecs/systems/aperture_system.hpp"
#include "core/ecs/systems/camera_focus_system.hpp"
#include "core/ecs/systems/physics/physics_system.hpp"

void Scene::init() {
    preUpdateScheduler.clear();
    preUpdateScheduler.add([this](ecs::Registry&) { ecs::animationSystem(*this); });
    preUpdateScheduler.add([this](ecs::Registry&) { ecs::physicsSystem(*this); });
    preUpdateScheduler.add([this](ecs::Registry&) { ecs::apertureSystem(registry, apertureState); });
    preUpdateScheduler.add(ecs::cameraFocusSystem);
}

void Scene::setActiveCamera(ecs::Entity newActiveCamera) {
    if (newActiveCamera == activeCamera) return;
    activeCamera = newActiveCamera;
    registry.markChanged(ecs::Camera);
}
