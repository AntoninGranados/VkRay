#include "physics_system.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_map>

#include "physics_solver.hpp"

#include "core/animation_handler.hpp"
#include "core/core.hpp"
#include "core/ecs/systems/animation_system.hpp"
#include "core/scene/scene.hpp"

namespace ecs {

namespace physics_detail {

static std::unordered_map<ecs::Entity, BodyState> gStates;
static std::unordered_map<ecs::Entity, physics_detail::ColliderState> gPrevColliderTransforms;
static int gPrevSolverFrame = -1;
static int gPrevSystemFrame = -1;
static bool gIsBaking = false;
struct BakeState {
    bool inProgress = false;
    int nextFrame = 0;
    int totalFrames = 0;
    int savedFrame = 0;
    bool wasPaused = true;
};
static BakeState gBakeState;

std::optional<ColliderState> getPrevColliderTransform(const Entity& entity) {
    auto it = gPrevColliderTransforms.find(entity);
    if (it == gPrevColliderTransforms.end()) return std::nullopt;
    return it->second;
}

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

void syncFromBody(
    const BodyAttributes& body,
    const glm::vec3& localCenterScaled,
    ecs::Component& transform,
    ecs::Component& rigidBody
) {
    const glm::quat newRot = glm::normalize(body.q);
    const glm::vec3 newPos = body.X - (newRot * localCenterScaled);
    transform.set<glm::vec3>("position", newPos);
    transform.set<glm::vec3>("rotation", glm::degrees(glm::eulerAngles(newRot)));
    rigidBody.set<glm::vec3>("linear_velocity", body.V);
    rigidBody.set<glm::vec3>("angular_velocity", body.omega);
}

FrameSnapshot captureFrameSnapshot(
    const BodyAttributes& body,
    const ecs::Component& transform,
    const ecs::Component& rigidBody
) {
    return FrameSnapshot{
        .body = captureBodySnapshot(body),
        .position = transform.get<glm::vec3>("position"),
        .rotation = glm::quat(glm::radians(transform.get<glm::vec3>("rotation"))),
        .linearVelocity = rigidBody.get<glm::vec3>("linear_velocity"),
        .angularVelocity = rigidBody.get<glm::vec3>("angular_velocity"),
    };
}

void applyFrameSnapshot(
    BodyAttributes& body,
    const FrameSnapshot& snapshot,
    ecs::Component& transform,
    ecs::Component& rigidBody
) {
    applyBodySnapshot(body, snapshot.body);
    transform.set<glm::vec3>("position", snapshot.position);
    transform.set<glm::vec3>("rotation", glm::degrees(glm::eulerAngles(glm::normalize(snapshot.rotation))));
    rigidBody.set<glm::vec3>("linear_velocity", snapshot.linearVelocity);
    rigidBody.set<glm::vec3>("angular_velocity", snapshot.angularVelocity);
}

} // namespace physics_detail

void physicsSolverSystem(Registry& registry) {
    using namespace ecs::physics_detail;

    auto& rigidBodies = registry.storage(RigidBody);
    auto& transforms = registry.storage(Transform);
    auto& meshes = registry.storage(MeshRef);
    auto& boxes = registry.storage(Box);
    auto& colliders = registry.storage(Collider);
    auto& meshAssets = Core::getScene().getMeshAssets();

    for (auto it = gStates.begin(); it != gStates.end();) {
        if (!rigidBodies.has(it->first) || !transforms.has(it->first) || (!meshes.has(it->first) && !boxes.has(it->first) && !registry.has(it->first, Sphere))) {
            it = gStates.erase(it);
        } else {
            ++it;
        }
    }

    const int currFrame = Core::getAnimation().getFrame();
    const float dt = static_cast<float>(Core::getAnimation().getFixedDt());
    const bool frameChanged = (currFrame != gPrevSolverFrame);
    gPrevSolverFrame = currFrame;

    bool changed = false;

    for (const ecs::Entity& e : rigidBodies.entities()) {
        if (!transforms.has(e)) continue;

        Component& rb = rigidBodies.get(e);
        Component& t = transforms.get(e);
        const bool hasMesh = meshes.has(e);
        const bool hasBox = boxes.has(e);
        const bool hasSphere = registry.has(e, Sphere);
        if (!hasMesh && !hasBox && !hasSphere) continue;

        MeshHandle meshHandle = -1;
        if (hasMesh) {
            meshHandle = meshes.get(e).get<int>("handle");
            if (meshHandle < 0 || static_cast<size_t>(meshHandle) >= meshAssets.size()) continue;
        }

        auto it = gStates.find(e);
        const bool needsInit = (
            it == gStates.end() ||
            !it->second.body
        );

        if (needsInit) {
            glm::vec3 localCenter(0.0f);
            glm::vec3 localHalfExtents(0.5f);
            if (hasMesh) {
                const MeshAsset& asset = meshAssets[meshHandle];
                glm::vec3 boundsMin(0.0f), boundsMax(0.0f);
                computeMeshBounds(asset, boundsMin, boundsMax);
                localCenter = 0.5f * (boundsMin + boundsMax);
                localHalfExtents = 0.5f * (boundsMax - boundsMin);
            } else if (hasBox) {
                localCenter = glm::vec3(0.0f);
                localHalfExtents = glm::vec3(1.0f);
            } else {
                localCenter = glm::vec3(0.0f);
                localHalfExtents = glm::vec3(registry.get(e, Sphere).get<float>("radius"));
            }

            const auto& safeExtent = [](float v) { return std::max(std::abs(v), 1e-3f); };

            const glm::vec3 tScale = t.get<glm::vec3>("scale");
            glm::vec3 scaledHalfExtents = glm::abs(tScale) * localHalfExtents;
            scaledHalfExtents.x = safeExtent(scaledHalfExtents.x);
            scaledHalfExtents.y = safeExtent(scaledHalfExtents.y);
            scaledHalfExtents.z = safeExtent(scaledHalfExtents.z);

            BodyState state;
            state.localCenterScaled = localCenter * tScale;
            const float density = rb.get<float>("density");
            const glm::vec3 linVel = rb.get<glm::vec3>("linear_velocity");
            const glm::vec3 angVel = rb.get<glm::vec3>("angular_velocity");

            if (hasSphere) {
                const float maxScale = std::max(std::max(std::abs(tScale.x), std::abs(tScale.y)), std::abs(tScale.z));
                const float radius = std::max(1e-4f, registry.get(e, Sphere).get<float>("radius") * maxScale);
                state.body = std::make_unique<RigidSphere>(
                    radius,
                    density,
                    glm::vec3(linVel.x, linVel.y, linVel.z),
                    glm::vec3(angVel.x, angVel.y, angVel.z)
                );
            } else {
                state.body = std::make_unique<RigidBox>(
                    scaledHalfExtents.x * 2.0f,
                    scaledHalfExtents.y * 2.0f,
                    scaledHalfExtents.z * 2.0f,
                    density,
                    glm::vec3(linVel.x, linVel.y, linVel.z),
                    glm::vec3(angVel.x, angVel.y, angVel.z)
                );
            }

            const glm::quat tQuat = glm::quat(glm::radians(t.get<glm::vec3>("rotation")));
            const glm::vec3 worldCenter = t.get<glm::vec3>("position") + (tQuat * state.localCenterScaled);
            state.body->X = glm::vec3(worldCenter.x, worldCenter.y, worldCenter.z);
            state.body->q = glm::normalize(tQuat);
            state.body->R = glm::mat3_cast(state.body->q);
            state.body->P = state.body->M * glm::vec3(linVel.x, linVel.y, linVel.z);
            state.body->Iinv = state.body->R * state.body->I0inv * glm::transpose(state.body->R);
            const glm::mat3 I = glm::inverse(state.body->Iinv);
            state.body->L = I * glm::vec3(angVel.x, angVel.y, angVel.z);

            state.solver.init(state.body.get());
            state.solver.setGravity(
                rb.get<bool>("use_gravity") ? glm::vec3(0.0f, -9.81f, 0.0f) : glm::vec3(0.0f, 0.0f, 0.0f)
            );
            state.initializedFrame = currFrame;
            state.snapshots.clear();
            state.snapshots.insert_or_assign(currFrame, captureFrameSnapshot(*state.body, t, rb));

            it = gStates.insert_or_assign(e, std::move(state)).first;
        }

        BodyState& state = it->second;
        if (!state.body) continue;

        // TODO: manually set the gravity so it is not an hardcoded value
        state.solver.setGravity(
            rb.get<bool>("use_gravity") ? glm::vec3(0.0f, -9.81f, 0.0f) : glm::vec3(0.0f, 0.0f, 0.0f)
        );

        auto exact = state.snapshots.find(currFrame);
        if (exact != state.snapshots.end()) {
            applyFrameSnapshot(*state.body, exact->second, t, rb);
            changed |= frameChanged;
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
            applyFrameSnapshot(*state.body, bestStart->second, t, rb);
        } else {
            startFrame = state.initializedFrame;
            auto initSnap = state.snapshots.find(startFrame);
            if (initSnap != state.snapshots.end()) {
                applyFrameSnapshot(*state.body, initSnap->second, t, rb);
            } else {
                continue;
            }
        }

        for (int f = startFrame + 1; f <= currFrame; ++f) {
            auto already = state.snapshots.find(f);
            if (already != state.snapshots.end()) {
                applyFrameSnapshot(*state.body, already->second, t, rb);
                continue;
            }
            state.solver.step(e, dt, registry);
            syncFromBody(*state.body, state.localCenterScaled, t, rb);
            state.snapshots.insert_or_assign(f, captureFrameSnapshot(*state.body, t, rb));
        }

        changed = true;
    }

    if (changed) Core::requestAccumulationRestart();

    if (gIsBaking) {
        for (const ecs::Entity& e : colliders.entities()) {
            if (!transforms.has(e)) continue;
            const Component& tc = transforms.get(e);
            gPrevColliderTransforms.insert_or_assign(e, ColliderState{
                .position = tc.get<glm::vec3>("position"),
                .rotation = tc.get<glm::vec3>("rotation"),
            });
        }
    }
}

void bakePhysicsSimulation(Registry& registry) {
    if (physics_detail::gBakeState.inProgress) return;

    AnimationHandler& animation = Core::getAnimation();
    const int endFrame = std::max(1, animation.getEndFrame());

    physics_detail::gStates.clear();
    physics_detail::gPrevColliderTransforms.clear();
    physics_detail::gPrevSolverFrame = -1;
    physics_detail::gPrevSystemFrame = -1;
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
        transformAnimationSystem(registry);
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

    static int prevFrame = 0;
    if (Core::getAnimation().isPaused() && prevFrame == Core::getAnimation().getFrame() && !Core::isAccumulationRestartPending()) return;
    prevFrame = Core::getAnimation().getFrame();

    auto& rigidBodies = registry.storage(RigidBody);
    auto& transforms = registry.storage(Transform);
    auto& meshes = registry.storage(MeshRef);
    auto& boxes = registry.storage(Box);

    // Remove states that no longer points to proper rigid bodies
    for (auto it = gStates.begin(); it != gStates.end();) {
        if (!rigidBodies.has(it->first) || !transforms.has(it->first)) {
            it = gStates.erase(it);
        } else if (!meshes.has(it->first) && !boxes.has(it->first) && !registry.has(it->first, Sphere)) {
            it = gStates.erase(it);
        } else {
            ++it;
        }
    }

    const int currFrame = Core::getAnimation().getFrame();
    const bool frameChanged = (currFrame != gPrevSystemFrame);
    gPrevSystemFrame = currFrame;

    // Apply the physics transform
    bool changed = false;
    for (const ecs::Entity& e : rigidBodies.entities()) {
        if (!transforms.has(e) || (!meshes.has(e) && !boxes.has(e) && !registry.has(e, Sphere))) continue;
        auto it = gStates.find(e);
        if (it == gStates.end()) continue;

        Component& t = transforms.get(e);
        Component& rb = rigidBodies.get(e);
        BodyState& state = it->second;
        auto snap = state.snapshots.find(currFrame);
        if (snap == state.snapshots.end()) continue;

        applyFrameSnapshot(*state.body, snap->second, t, rb);
        changed |= frameChanged;
    }

    if (changed) {
        Core::requestAccumulationRestart();
    }
}

} // namespace ecs
