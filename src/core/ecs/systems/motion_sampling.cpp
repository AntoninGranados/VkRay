#include "motion_sampling.hpp"

#include <algorithm>
#include <cmath>
#include <random>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "core/animation/animation_store.hpp"
#include "core/camera/camera.hpp"
#include "core/core.hpp"
#include "core/ecs/components/camera.hpp"
#include "core/ecs/components/component.hpp"
#include "core/ecs/components/physics.hpp"
#include "core/ecs/systems/physics/physics_solver.hpp"
#include "core/scene/gpu_structs.hpp"

namespace ecs {

namespace {

bool isEntityInMotion(Registry& registry, Entity entity, Component& transform) {
    if (registry.has(entity, RigidBody)) return true;
    AnimationStore& store = *registry.ctx().get<AnimationStore*>();
    return !store.keyframes(transform.getField("position")).empty()
        || !store.keyframes(transform.getField("rotation")).empty()
        || !store.keyframes(transform.getField("scale")).empty();
}

GpuMotionSample sampleFromValues(const glm::vec3& position, const glm::vec3& rotationEuler, const glm::vec3& scale) {
    const glm::quat rotation = glm::quat(glm::radians(rotationEuler));
    return GpuMotionSample{
        .rotation = glm::vec4(rotation.x, rotation.y, rotation.z, rotation.w),
        .translation = position,
        .scale = scale,
    };
}

GpuMotionSample sampleLiveTransform(Component& transform) {
    return sampleFromValues(transform.get<glm::vec3>("position"), transform.get<glm::vec3>("rotation"), transform.get<glm::vec3>("scale"));
}

GpuMotionSample sampleTrackAt(AnimationStore& store, Component& transform, float frame, float anchorFrame) {
    Field& positionField = transform.getField("position");
    Field& rotationField = transform.getField("rotation");
    Field& scaleField = transform.getField("scale");

    const glm::vec3 position = transform.get<glm::vec3>("position")
        + (store.sampleAt<glm::vec3>(positionField, frame) - store.sampleAt<glm::vec3>(positionField, anchorFrame));

    const glm::quat rotationDelta = glm::quat(glm::radians(store.sampleAt<glm::vec3>(rotationField, frame)))
        * glm::inverse(glm::quat(glm::radians(store.sampleAt<glm::vec3>(rotationField, anchorFrame))));
    const glm::quat rotation = rotationDelta * glm::quat(glm::radians(transform.get<glm::vec3>("rotation")));

    const glm::vec3 scaleAtFrame = store.sampleAt<glm::vec3>(scaleField, frame);
    const glm::vec3 scaleAtAnchor = store.sampleAt<glm::vec3>(scaleField, anchorFrame);
    const glm::vec3 scaleRatio = glm::vec3(
        scaleAtAnchor.x != 0.0f ? scaleAtFrame.x / scaleAtAnchor.x : 1.0f,
        scaleAtAnchor.y != 0.0f ? scaleAtFrame.y / scaleAtAnchor.y : 1.0f,
        scaleAtAnchor.z != 0.0f ? scaleAtFrame.z / scaleAtAnchor.z : 1.0f
    );
    const glm::vec3 scale = transform.get<glm::vec3>("scale") * scaleRatio;

    return GpuMotionSample{
        .rotation = glm::vec4(rotation.x, rotation.y, rotation.z, rotation.w),
        .translation = position,
        .scale = scale,
    };
}

GpuMotionSample sampleRigidBodyAt(Registry& registry, Entity entity, Component& transform, float frame) {
    physics_detail::BodyState& state = registry.get(entity, RigidBody).payload<physics_detail::BodyState>("state");
    const glm::vec3 scale = transform.get<glm::vec3>("scale");
    if (state.snapshots.empty())
        return sampleFromValues(transform.get<glm::vec3>("position"), transform.get<glm::vec3>("rotation"), scale);

    const auto [minFrame, maxFrame] = state.snapshotFrameRange();
    const float clamped = std::clamp(frame, static_cast<float>(minFrame), static_cast<float>(maxFrame));
    const int frameLo = static_cast<int>(std::floor(clamped));
    const float alpha = clamped - static_cast<float>(frameLo);

    const auto snapLo = state.snapshots.find(frameLo);
    if (snapLo == state.snapshots.end())
        return sampleFromValues(transform.get<glm::vec3>("position"), transform.get<glm::vec3>("rotation"), scale);

    const auto snapHi = alpha > 0.0f ? state.snapshots.find(frameLo + 1) : state.snapshots.end();
    const glm::vec3 position = snapHi == state.snapshots.end()
        ? snapLo->second.position
        : glm::mix(snapLo->second.position, snapHi->second.position, alpha);
    const glm::quat rotation = snapHi == state.snapshots.end()
        ? snapLo->second.rotation
        : glm::slerp(snapLo->second.rotation, snapHi->second.rotation, alpha);

    return GpuMotionSample{
        .rotation = glm::vec4(rotation.x, rotation.y, rotation.z, rotation.w),
        .translation = position,
        .scale = scale,
    };
}

} // namespace

void bakeLiveTransform(Component& transform, std::vector<GpuMotionSample>& outMotion) {
    outMotion.push_back(sampleLiveTransform(transform));
}

uint32_t bakeMotionSamples(Registry& registry, Entity entity, Component& transform, std::vector<GpuMotionSample>& outMotion) {
    const uint32_t offset = static_cast<uint32_t>(outMotion.size());

    const Entity cameraEntity = *registry.ctx().get<Entity*>();
    const float shutterSpeed = registry.get(cameraEntity, Camera).get<float>("shutter_speed");
    const bool inMotion = isEntityInMotion(registry, entity, transform);

    if (shutterSpeed <= 0.0f || !inMotion) {
        outMotion.push_back(sampleLiveTransform(transform));
        return offset;
    }

    static std::mt19937 rng{ std::random_device{}() };
    const int frame = Core::getAnimation().getFrame();
    const float halfBlur = blurFractionFromShutter(shutterSpeed, static_cast<float>(Core::getAnimation().getFps())) * 0.5f;
    std::uniform_real_distribution<float> dist(-halfBlur, halfBlur);
    const float sampleFrame = static_cast<float>(frame) + dist(rng);

    outMotion.push_back(registry.has(entity, RigidBody)
        ? sampleRigidBodyAt(registry, entity, transform, sampleFrame)
        : sampleTrackAt(*registry.ctx().get<AnimationStore*>(), transform, sampleFrame, static_cast<float>(frame)));
    return offset;
}

} // namespace ecs
