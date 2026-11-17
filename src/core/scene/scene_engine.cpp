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

void Scene::init(SceneGpuBuffers& buffers) {
    preUpdateScheduler.clear();
    preUpdateScheduler.add([this](ecs::Registry&) { ecs::animationSystem(*this); });
    preUpdateScheduler.add([this](ecs::Registry&) { ecs::physicsSystem(*this); });
    preUpdateScheduler.add([this](ecs::Registry&) { ecs::apertureSystem(registry, apertureState); });
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
    onRenderScheduler.add([this, &buffers](ecs::Registry&) { ecs::texturePackingSystem(*this, buffers); },
                          {&ecs::Texture});
    onRenderScheduler.add([this, &buffers](ecs::Registry&) { ecs::materialPackingSystem(*this, buffers); },
                          join({materials, cameras, environments, {&ecs::Compositing, &ecs::Texture}}));
    onRenderScheduler.add([this, &buffers](ecs::Registry&) { ecs::meshPackingSystem(*this, buffers); }, {&ecs::Mesh});
    onRenderScheduler.add([this, &buffers](ecs::Registry&) { ecs::meshInstancePackingSystem(*this, buffers); },
                          {&ecs::Mesh, &ecs::MeshRef, &ecs::Transform});
    onRenderScheduler.add([this, &buffers](ecs::Registry&) { ecs::objectPackingSystem(*this, buffers); },
                          join({objects, materials, {&ecs::Transform, &ecs::Camera, &ecs::RigidBody, &ecs::Mesh}}),
                          [this](ecs::Registry&) { return ecs::isMotionBlurActive(*this); });
    onRenderScheduler.add([&buffers](ecs::Registry& registry) { ecs::lightPackingSystem(registry, buffers); },
                          join({objects, materials, {&ecs::Transform, &ecs::Mesh}}));
}

void Scene::setActiveCamera(ecs::Entity newActiveCamera) {
    if (newActiveCamera == activeCamera) return;
    activeCamera = newActiveCamera;
    registry.markChanged(ecs::Camera);
}
