#include "physics_system.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

#include "core/animation/animation_handler.hpp"
#include "core/core.hpp"
#include "physics_solver.hpp"
#include "core/ecs/systems/animation_system.hpp"
#include "core/scene/scene.hpp"

namespace ecs {

namespace physics_detail {

static bool gIsBaking = false;
struct BakeState {
    bool inProgress = false;
    int nextFrame = 0;
    int totalFrames = 0;
    int savedFrame = 0;
    bool wasPaused = true;
};
static BakeState gBakeState;

void computeMeshBounds(const MeshAsset& mesh, glm::vec3& outMin, glm::vec3& outMax) {
    const auto& vertices = mesh.getVertices();
    if (vertices.empty()) {
        outMin = glm::vec3(-0.5f);
        outMax = glm::vec3(0.5f);
        return;
    }

    outMin = vertices[0].position;
    outMax = vertices[0].position;
    for (size_t i = 1; i < vertices.size(); ++i) {
        outMin = glm::min(outMin, vertices[i].position);
        outMax = glm::max(outMax, vertices[i].position);
    }
}

BodySnapshot captureBodySnapshot(const BodyAttributes& body) {
    return BodySnapshot{
        .X = body.X,
        .R = body.R,
        .q = body.q,
        .P = body.P,
        .L = body.L,
        .V = body.V,
        .omega = body.omega,
        .Iinv = body.Iinv,
    };
}

void applyBodySnapshot(BodyAttributes& body, const BodySnapshot& snapshot) {
    body.X = snapshot.X;
    body.R = snapshot.R;
    body.q = snapshot.q;
    body.P = snapshot.P;
    body.L = snapshot.L;
    body.V = snapshot.V;
    body.omega = snapshot.omega;
    body.Iinv = snapshot.Iinv;
}

void syncFromBody(const BodyAttributes& body, const glm::vec3& localCenterScaled, ecs::Component& transform) {
    const glm::quat newRot = glm::normalize(body.q);
    const glm::vec3 newPos = body.X - (newRot * localCenterScaled);
    transform.set<glm::vec3>("position", newPos);
    transform.set<glm::vec3>("rotation", glm::degrees(glm::eulerAngles(newRot)));
}

FrameSnapshot captureFrameSnapshot(const BodyAttributes& body, const ecs::Component& transform) {
    return FrameSnapshot{
        .body = captureBodySnapshot(body),
        .position = transform.get<glm::vec3>("position"),
        .rotation = glm::quat(glm::radians(transform.get<glm::vec3>("rotation"))),
    };
}

void applyFrameSnapshot(BodyAttributes& body, const FrameSnapshot& snapshot, ecs::Component& transform) {
    applyBodySnapshot(body, snapshot.body);
    transform.set<glm::vec3>("position", snapshot.position);
    transform.set<glm::vec3>("rotation", glm::degrees(glm::eulerAngles(glm::normalize(snapshot.rotation))));
}

} // namespace physics_detail

void physicsSolverSystem(Registry& registry) {
    using namespace ecs::physics_detail;

    auto& rigidBodies = registry.storage(RigidBody);
    auto& transforms = registry.storage(Transform);
    auto& meshes = registry.storage(MeshRef);
    auto& boxes = registry.storage(Box);
    auto& colliders = registry.storage(Collider);

    const int currFrame = Core::getAnimation().getFrame();
    const float dt = static_cast<float>(Core::getAnimation().getFixedDt());

    for (const ecs::Entity& e : rigidBodies.entities()) {
        if (!transforms.has(e)) continue;

        Component& rb = rigidBodies.get(e);
        Component& t = transforms.get(e);
        const bool hasMesh = meshes.has(e);
        const bool hasBox = boxes.has(e);
        const bool hasSphere = registry.has(e, Sphere);
        if (!hasMesh && !hasBox && !hasSphere) continue;

        const MeshAsset* meshAsset = nullptr;
        if (hasMesh) {
            const ecs::Entity meshEntity = meshes.get(e).get<ecs::Entity>("handle");
            meshAsset = Core::getScene().getMeshAsset(meshEntity);
            if (!meshAsset) continue;
        }

        BodyState& state = rb.payload<BodyState>("state");
        const bool needsInit = !state.body;

        if (needsInit) {
            glm::vec3 localCenter(0.0f);
            glm::vec3 localHalfExtents(0.5f);
            if (hasMesh) {
                glm::vec3 boundsMin(0.0f), boundsMax(0.0f);
                computeMeshBounds(*meshAsset, boundsMin, boundsMax);
                localCenter = 0.5f * (boundsMin + boundsMax);
                localHalfExtents = 0.5f * (boundsMax - boundsMin);
            } else if (hasBox) {
                localCenter = glm::vec3(0.0f);
                localHalfExtents = glm::vec3(1.0f);
            } else {
                localCenter = glm::vec3(0.0f);
                localHalfExtents = glm::vec3(registry.get(e, Sphere).get<float>("radius"));
            }

            const auto safeExtent = [](float v) { return std::max(std::abs(v), 1e-3f); };

            const glm::vec3 tScale = t.get<glm::vec3>("scale");
            glm::vec3 scaledHalfExtents = glm::abs(tScale) * localHalfExtents;
            scaledHalfExtents.x = safeExtent(scaledHalfExtents.x);
            scaledHalfExtents.y = safeExtent(scaledHalfExtents.y);
            scaledHalfExtents.z = safeExtent(scaledHalfExtents.z);

            state.localCenterScaled = localCenter * tScale;
            const float density = rb.get<float>("density");
            const glm::vec3 linVel(0.0f);
            const glm::vec3 angVel(0.0f);

            if (hasSphere) {
                const float maxScale = std::max(std::max(std::abs(tScale.x), std::abs(tScale.y)), std::abs(tScale.z));
                const float radius = std::max(1e-4f, registry.get(e, Sphere).get<float>("radius") * maxScale);
                state.body = std::make_unique<RigidSphere>(radius, density, linVel, angVel);
            } else {
                state.body = std::make_unique<RigidBox>(
                    scaledHalfExtents.x * 2.0f,
                    scaledHalfExtents.y * 2.0f,
                    scaledHalfExtents.z * 2.0f,
                    density,
                    linVel,
                    angVel
                );
            }

            const glm::quat tQuat = glm::quat(glm::radians(t.get<glm::vec3>("rotation")));
            state.body->X = t.get<glm::vec3>("position") + (tQuat * state.localCenterScaled);
            state.body->q = glm::normalize(tQuat);
            state.body->R = glm::mat3_cast(state.body->q);
            state.body->P = state.body->M * linVel;
            state.body->Iinv = state.body->R * state.body->I0inv * glm::transpose(state.body->R);
            const glm::mat3 I = glm::inverse(state.body->Iinv);
            state.body->L = I * angVel;

            state.solver.init(state.body.get());
            state.solver.setGravity(
                rb.get<bool>("use_gravity") ? glm::vec3(0.0f, -9.81f, 0.0f) : glm::vec3(0.0f, 0.0f, 0.0f)
            );
            state.initializedFrame = currFrame;
            state.snapshots.clear();
            state.snapshots.insert_or_assign(currFrame, captureFrameSnapshot(*state.body, t));
        }

        if (!state.body) continue;

        // TODO: manually set the gravity so it is not an hardcoded value
        state.solver.setGravity(
            rb.get<bool>("use_gravity") ? glm::vec3(0.0f, -9.81f, 0.0f) : glm::vec3(0.0f, 0.0f, 0.0f)
        );

        auto exact = state.snapshots.find(currFrame);
        if (exact != state.snapshots.end()) {
            applyFrameSnapshot(*state.body, exact->second, t);
            continue;
        }

        // Should only be applied by the baking function
        if (!gIsBaking) continue;

        int startFrame = state.initializedFrame;
        auto bestStart = state.snapshots.end();
        for (auto itSnap = state.snapshots.begin(); itSnap != state.snapshots.end(); ++itSnap) {
            if (itSnap->first < currFrame && (bestStart == state.snapshots.end() || itSnap->first > bestStart->first)) {
                bestStart = itSnap;
            }
        }

        if (bestStart != state.snapshots.end()) {
            startFrame = bestStart->first;
            applyFrameSnapshot(*state.body, bestStart->second, t);
        } else {
            startFrame = state.initializedFrame;
            auto initSnap = state.snapshots.find(startFrame);
            if (initSnap != state.snapshots.end()) {
                applyFrameSnapshot(*state.body, initSnap->second, t);
            } else {
                continue;
            }
        }

        for (int f = startFrame + 1; f <= currFrame; ++f) {
            auto already = state.snapshots.find(f);
            if (already != state.snapshots.end()) {
                applyFrameSnapshot(*state.body, already->second, t);
                continue;
            }
            state.solver.step(e, dt, registry);
            syncFromBody(*state.body, state.localCenterScaled, t);
            state.snapshots.insert_or_assign(f, captureFrameSnapshot(*state.body, t));
        }
    }

    if (gIsBaking) {
        for (const ecs::Entity& e : colliders.entities()) {
            if (!transforms.has(e)) continue;
            const Component& tc = transforms.get(e);
            colliders.get(e).payload<ColliderState>("prev_transform") = ColliderState{
                .position = tc.get<glm::vec3>("position"),
                .rotation = tc.get<glm::vec3>("rotation"),
                .valid = true,
            };
        }
    }
}

void bakePhysicsSimulation(Registry& registry) {
    if (physics_detail::gBakeState.inProgress) return;

    AnimationHandler& animation = Core::getAnimation();
    const int endFrame = std::max(1, animation.getEndFrame());

    for (const Entity& e : registry.storage(RigidBody).entities()) {
        physics_detail::BodyState fresh;
        registry.get(e, RigidBody).payload<physics_detail::BodyState>("state") = std::move(fresh);
    }
    for (const Entity& e : registry.storage(Collider).entities())
        registry.get(e, Collider).payload<physics_detail::ColliderState>("prev_transform") = physics_detail::ColliderState{};

    physics_detail::gIsBaking = true;
    physics_detail::gBakeState.inProgress = true;
    physics_detail::gBakeState.nextFrame = 0;
    physics_detail::gBakeState.totalFrames = endFrame;
    physics_detail::gBakeState.savedFrame = animation.getFrame();
    physics_detail::gBakeState.wasPaused = animation.isPaused();

    animation.pause();
    Core::requestAccumulationRestart();
}

bool isPhysicsBakeInProgress() {
    return physics_detail::gBakeState.inProgress;
}

int getPhysicsBakeCurrentFrame() {
    return physics_detail::gBakeState.nextFrame;
}

int getPhysicsBakeTotalFrames() {
    return physics_detail::gBakeState.totalFrames;
}

void physicsSystem(Registry& registry) {
    using namespace ecs::physics_detail;

    if (gBakeState.inProgress) {
        Core::getAnimation().reset(gBakeState.nextFrame);
        Core::getAnimation().sample();
        evaluateAnimation(registry);
        physicsSolverSystem(registry);
        Core::requestAccumulationRestart();

        gBakeState.nextFrame++;
        if (gBakeState.nextFrame >= gBakeState.totalFrames) {
            gIsBaking = false;
            gBakeState.inProgress = false;
            Core::getAnimation().reset(gBakeState.savedFrame);
            if (!gBakeState.wasPaused) Core::getAnimation().play();
        } else {
            Core::getAnimation().reset(gBakeState.savedFrame);
        }
        return;
    }

    auto& rigidBodies = registry.storage(RigidBody);
    if (rigidBodies.entities().empty()) return;
    if (!Core::getAnimation().didFrameChange()) return;

    auto& transforms = registry.storage(Transform);
    auto& meshes = registry.storage(MeshRef);
    auto& boxes = registry.storage(Box);

    const float sampleFrame = Core::getAnimation().getSampleFrame();

    // Apply the physics transform
    for (const ecs::Entity& e : rigidBodies.entities()) {
        if (!transforms.has(e) || (!meshes.has(e) && !boxes.has(e) && !registry.has(e, Sphere))) continue;

        Component& t = transforms.get(e);
        BodyState& state = rigidBodies.get(e).payload<BodyState>("state");
        if (state.snapshots.empty()) continue;

        int minFrame = state.snapshots.begin()->first;
        int maxFrame = minFrame;
        for (const auto& [f, snap] : state.snapshots) {
            minFrame = std::min(minFrame, f);
            maxFrame = std::max(maxFrame, f);
        }

        const float clampedSample = std::clamp(sampleFrame, static_cast<float>(minFrame), static_cast<float>(maxFrame));
        const int frameLo = static_cast<int>(std::floor(clampedSample));
        const float alpha = clampedSample - static_cast<float>(frameLo);

        auto snapLo = state.snapshots.find(frameLo);
        if (snapLo == state.snapshots.end()) continue;

        auto snapHi = alpha > 0.0f ? state.snapshots.find(frameLo + 1) : state.snapshots.end();
        if (snapHi == state.snapshots.end()) {
            t.set<glm::vec3>("position", snapLo->second.position);
            t.set<glm::vec3>("rotation", glm::degrees(glm::eulerAngles(glm::normalize(snapLo->second.rotation))));
        } else {
            t.set<glm::vec3>("position", glm::mix(snapLo->second.position, snapHi->second.position, alpha));
            t.set<glm::vec3>("rotation", glm::degrees(glm::eulerAngles(
                glm::slerp(snapLo->second.rotation, snapHi->second.rotation, alpha))));
        }
    }

    Core::getScene().touch();
}

} // namespace ecs
