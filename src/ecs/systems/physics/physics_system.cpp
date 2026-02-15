#include "physics_system.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_map>

#include "physics_solver.hpp"

#include "../../components.hpp"
#include "../animation_system.hpp"
#include "../transform_system.hpp"
#include "../../../scene/scene.hpp"
#include "../../../animation_handler.hpp"

namespace ecs {

namespace physics_detail {

static std::unordered_map<ecs::Entity, SolverState> gStates;
static int gPrevSolverFrame = -1;
static int gPrevSystemFrame = -1;
static bool gIsBaking = false;

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
    ecs::Transform& transform,
    ecs::RigidBody& rigidBody
) {
    const glm::quat newRot = glm::normalize(body.q);
    const glm::vec3 center = body.X;
    const glm::vec3 newPos = center - (newRot * localCenterScaled);
    transform.setRotation(newRot);
    transform.setPosition(newPos);
    rigidBody.linearVelocity = body.V;
    rigidBody.angularVelocity = body.omega;
}

FrameSnapshot captureFrameSnapshot(
    const BodyAttributes& body,
    const ecs::Transform& transform,
    const ecs::RigidBody& rigidBody
) {
    return FrameSnapshot{
        .body = captureBodySnapshot(body),
        .position = transform.position,
        .rotation = transform.rotation,
        .linearVelocity = rigidBody.linearVelocity,
        .angularVelocity = rigidBody.angularVelocity,
    };
}

void applyFrameSnapshot(
    BodyAttributes& body,
    const FrameSnapshot& snapshot,
    ecs::Transform& transform,
    ecs::RigidBody& rigidBody
) {
    applyBodySnapshot(body, snapshot.body);
    transform.setPosition(snapshot.position);
    transform.setRotation(snapshot.rotation);
    rigidBody.linearVelocity = snapshot.linearVelocity;
    rigidBody.angularVelocity = snapshot.angularVelocity;
}

} // namespace physics_detail

void physicsSolver(Registry& registry, AppContext& ctx) {
    using namespace ecs::physics_detail;

    if (!ctx.animation) return;

    auto& rigidBodies = registry.storage<ecs::RigidBody>();
    auto& transforms = registry.storage<ecs::Transform>();
    auto& meshRefs = registry.storage<ecs::MeshRef>();
    auto& boxes = registry.storage<ecs::Box>();
    auto& meshAssets = ctx.scene->getMeshAssets();

    auto& planes = registry.storage<ecs::Plane>();
    auto& planeColliders = registry.storage<ecs::PlaneCollider>();

    for (auto it = gStates.begin(); it != gStates.end();) {
        if (!rigidBodies.has(it->first) || !transforms.has(it->first) || (!meshRefs.has(it->first) && !boxes.has(it->first))) {
            it = gStates.erase(it);
        } else {
            ++it;
        }
    }

    const int currFrame = ctx.animation->getFrame();
    const float dt = static_cast<float>(ctx.animation->getFixedDt());
    const bool frameChanged = (currFrame != gPrevSolverFrame);
    gPrevSolverFrame = currFrame;

    bool changed = false;

    for (const ecs::Entity& e : rigidBodies.entities()) {
        if (!transforms.has(e)) continue;

        ecs::RigidBody& rb = rigidBodies.get(e);
        ecs::Transform& t = transforms.get(e);
        const bool hasMesh = meshRefs.has(e);
        const bool hasBox = boxes.has(e);
        if (!hasMesh && !hasBox) continue;

        MeshHandle meshHandle = -1;
        if (hasMesh) {
            meshHandle = meshRefs.get(e).handle;
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
            } else {
                localCenter = glm::vec3(0.0f);
                localHalfExtents = glm::vec3(1.0f);
            }

            const auto& safeExtent = [](float v) { return std::max(std::abs(v), 1e-3f); };

            glm::vec3 scaledHalfExtents = glm::abs(t.scale) * localHalfExtents;
            scaledHalfExtents.x = safeExtent(scaledHalfExtents.x);
            scaledHalfExtents.y = safeExtent(scaledHalfExtents.y);
            scaledHalfExtents.z = safeExtent(scaledHalfExtents.z);

            SolverState state;
            state.localCenterScaled = localCenter * t.scale;
            state.body = std::make_unique<RigidBox>(
                scaledHalfExtents.x * 2.0f,
                scaledHalfExtents.y * 2.0f,
                scaledHalfExtents.z * 2.0f,
                rb.density,
                glm::vec3(rb.linearVelocity.x, rb.linearVelocity.y, rb.linearVelocity.z),
                glm::vec3(rb.angularVelocity.x, rb.angularVelocity.y, rb.angularVelocity.z)
            );

            const glm::vec3 worldCenter = t.position + (t.rotation * state.localCenterScaled);
            state.body->X = glm::vec3(worldCenter.x, worldCenter.y, worldCenter.z);
            state.body->q = glm::normalize(t.rotation);
            state.body->R = glm::mat3_cast(state.body->q);
            state.body->P = state.body->M * glm::vec3(rb.linearVelocity.x, rb.linearVelocity.y, rb.linearVelocity.z);
            state.body->Iinv = state.body->R * state.body->I0inv * glm::transpose(state.body->R);
            const glm::mat3 I = glm::inverse(state.body->Iinv);
            state.body->L = I * glm::vec3(rb.angularVelocity.x, rb.angularVelocity.y, rb.angularVelocity.z);

            state.solver.init(state.body.get());
            state.solver.setGravity(
                rb.useGravity ? glm::vec3(0.0f, -9.81f, 0.0f) : glm::vec3(0.0f, 0.0f, 0.0f)
            );
            state.initializedFrame = currFrame;
            state.snapshots.clear();
            state.snapshots.insert_or_assign(currFrame, captureFrameSnapshot(*state.body, t, rb));

            it = gStates.insert_or_assign(e, std::move(state)).first;
        }

        SolverState& state = it->second;
        if (!state.body) continue;

        state.solver.setGravity(
            rb.useGravity ? glm::vec3(0.0f, -9.81f, 0.0f) : glm::vec3(0.0f, 0.0f, 0.0f)
        );

        auto exact = state.snapshots.find(currFrame);
        if (exact != state.snapshots.end()) {
            applyFrameSnapshot(*state.body, exact->second, t, rb);
            changed |= frameChanged;
            continue;
        }

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
            state.solver.step(dt, planeColliders, planes, transforms);
            syncFromBody(*state.body, state.localCenterScaled, t, rb);
            state.snapshots.insert_or_assign(f, captureFrameSnapshot(*state.body, t, rb));
        }

        changed = true;
    }

    if (changed) {
        *ctx.restartRender = true;
    }
}

void bakePhysicsSimulation(Registry& registry, AppContext& ctx) {
    if (!ctx.animation) return;

    const int savedFrame = ctx.animation->getFrame();
    const bool wasPaused = ctx.animation->isPaused();
    const int endFrame = ctx.animation->getEndFrame();

    physics_detail::gStates.clear();
    physics_detail::gPrevSolverFrame = -1;
    physics_detail::gPrevSystemFrame = -1;
    physics_detail::gIsBaking = true;

    ctx.animation->pause();
    for (int f = 0; f < endFrame; ++f) {
        ctx.animation->reset(f);
        transformAnimationSystem(registry, ctx);
        transformSystem(registry, ctx);
        physicsSolver(registry, ctx);
    }
    physics_detail::gIsBaking = false;

    ctx.animation->reset(savedFrame);
    if (!wasPaused) ctx.animation->play();

    *ctx.restartRender = true;
}

void physicsSystem(Registry& registry, AppContext& ctx) {
    using namespace ecs::physics_detail;

    if (!ctx.animation) return;

    auto& rigidBodies = registry.storage<ecs::RigidBody>();
    auto& transforms = registry.storage<ecs::Transform>();
    auto& meshRefs = registry.storage<ecs::MeshRef>();
    auto& boxes = registry.storage<ecs::Box>();

    // Remove states that no longer points to proper rigid bodies
    for (auto it = gStates.begin(); it != gStates.end();) {
        if (!rigidBodies.has(it->first) || !transforms.has(it->first) || (!meshRefs.has(it->first) && !boxes.has(it->first))) {
            it = gStates.erase(it);
        } else {
            ++it;
        }
    }

    const int currFrame = ctx.animation->getFrame();
    const bool frameChanged = (currFrame != gPrevSystemFrame);
    gPrevSystemFrame = currFrame;

    // Apply the physics transform
    bool changed = false;
    for (const ecs::Entity& e : rigidBodies.entities()) {
        if (!transforms.has(e) || (!meshRefs.has(e) && !boxes.has(e))) continue;
        auto it = gStates.find(e);
        if (it == gStates.end()) continue;

        ecs::Transform& t = transforms.get(e);
        ecs::RigidBody& rb = rigidBodies.get(e);
        SolverState& state = it->second;
        auto snap = state.snapshots.find(currFrame);
        if (snap == state.snapshots.end()) continue;

        applyFrameSnapshot(*state.body, snap->second, t, rb);
        changed |= frameChanged;
    }

    if (changed) {
        *ctx.restartRender = true;
    }
}

} // namespace ecs
