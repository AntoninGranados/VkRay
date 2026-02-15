#include "physics_solver_system.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_map>

#include <glm/gtc/quaternion.hpp>

#include "../components.hpp"
#include "../../scene/scene.hpp"
#include "../../animation_handler.hpp"

namespace ecs {

namespace physics_detail {

BodyAttributes::BodyAttributes() :
    M(1.0f),
    I0(1.0f), I0inv(1.0f), Iinv(1.0f),
    X(0, 0, 0), R(1.0f), q(1, 0, 0, 0), P(0, 0, 0), L(0, 0, 0),
    V(0, 0, 0), omega(0, 0, 0), F(0, 0, 0), tau(0, 0, 0) {}

RigidBox::RigidBox(
    const float w, const float h, const float d, const float dens,
    const glm::vec3 v0, const glm::vec3 omega0) : width(w), height(h), depth(d)
{
    V = v0;
    omega = omega0;

    M = dens * width * height * depth;
    glm::vec3 diag = glm::vec3(
        height * height + depth * depth,
        width * width + depth * depth,
        width * width + height * height
    );
    diag = diag * M / 12.0f;
    I0 = glm::mat3(diag.x, 0, 0, 0, diag.y, 0, 0, 0, diag.z);
    I0inv = glm::inverse(I0);
    Iinv = R * I0inv * glm::transpose(R);

    vdata0.push_back(glm::vec3(-0.5f * w, -0.5f * h, -0.5f * d));
    vdata0.push_back(glm::vec3( 0.5f * w, -0.5f * h, -0.5f * d));
    vdata0.push_back(glm::vec3( 0.5f * w,  0.5f * h, -0.5f * d));
    vdata0.push_back(glm::vec3(-0.5f * w,  0.5f * h, -0.5f * d));
    vdata0.push_back(glm::vec3(-0.5f * w, -0.5f * h,  0.5f * d));
    vdata0.push_back(glm::vec3( 0.5f * w, -0.5f * h,  0.5f * d));
    vdata0.push_back(glm::vec3( 0.5f * w,  0.5f * h,  0.5f * d));
    vdata0.push_back(glm::vec3(-0.5f * w,  0.5f * h,  0.5f * d));
}

RigidSolver::RigidSolver(BodyAttributes* body0, const glm::vec3 g) :
    body(body0), gravity(g), stepCount(0), simTime(0), appTime(0) {}

void RigidSolver::init(BodyAttributes* body0) {
    body = body0;
    stepCount = 0;
    simTime = 0;
    appTime = 0;
}

void RigidSolver::setGravity(const glm::vec3& g) {
    gravity = g;
}

void RigidSolver::step(
    const float dt,
    const ComponentStorage<PlaneCollider>& planeColliders,
    const ComponentStorage<Plane>& planes,
    const ComponentStorage<Transform>& transforms
) {
    if (!body || dt <= 0.0f) return;
    appTime += dt;

    while (appTime > simTime) {
        computeForceAndTorque();
        integrate(simDt * 0.5f);
        for (const Entity& e : planeColliders.entities()) {
            if (!planes.has(e) || !transforms.has(e)) continue;
            const PlaneCollider& collider = planeColliders.get(e);
            const Transform& planeTransform = transforms.get(e);
            const glm::vec3 normal = glm::normalize(planeTransform.rotation * glm::vec3(0.0f, 1.0f, 0.0f));
            resolvePlaneCollision(planeTransform.position, normal, collider.restitution, collider.friction);
        }
        integrate(simDt * 0.5f);
        ++stepCount;
    }
}

void RigidSolver::integrate(const float dt) {
    body->P += dt * body->F;
    body->L += dt * body->tau;

    body->V = body->P / body->M;
    body->X += dt * body->V;

    body->Iinv = body->R * body->I0inv * glm::transpose(body->R);
    body->omega = body->Iinv * body->L;
    glm::quat omegaTilde(0.0f, body->omega.x, body->omega.y, body->omega.z);
    body->q += dt * 0.5f * (omegaTilde * body->q);
    body->q = glm::normalize(body->q);
    body->R = glm::mat3_cast(body->q);

    simTime += dt;
}

void RigidSolver::computeForceAndTorque() {
    body->F = glm::vec3(0.0f);
    body->F += body->M * gravity;

    if (stepCount == 1) {
        const glm::vec3 J = glm::vec3(0.0f, 0.6f, -1.0f) * 0.05f;
        body->P += J;

        const glm::vec3 p = body->R * body->vdata0[0];
        const glm::vec3 tauImpulse = glm::cross(p, J) * 0.1f;
        body->L += tauImpulse;
    }
}

void RigidSolver::resolvePlaneCollision(const glm::vec3& p0, const glm::vec3& n, float eps, float mu) {
    for (size_t i = 0; i < body->vdata0.size(); i++) {
        const glm::vec3 rRel = body->R * body->vdata0[i];
        const glm::vec3 r = rRel + body->X;
        const float d = glm::dot(n, r - p0);
        const float dist = std::abs(d);
        const glm::vec3 normal = (d >= 0.0f) ? n : -n;

        const glm::vec3 v = body->V + glm::cross(body->omega, rRel);
        const float vRel = glm::dot(normal, v);
        if (vRel >= 1e-1f) continue; // moving away from plane

        const float approach = -vRel * simDt;
        if (dist > approach + 1e-3f) continue;

        const glm::vec3 rCrossN = glm::cross(rRel, normal);
        const float denom = 1.0f / body->M + glm::dot(rCrossN, body->Iinv * rCrossN);
        if (!std::isfinite(denom) || denom <= 1e-8f) continue;
        const float j = -(1.0f + eps) * vRel / denom;
        if (!std::isfinite(j) || j <= 0.0f) continue;

        glm::vec3 J = j * normal;
        const glm::vec3 vT = v - glm::dot(v, normal) * normal;
        const float vTlen = glm::length(vT);
        if (vTlen >= 1e-8f) {
            const glm::vec3 t = vT / vTlen;
            const glm::vec3 rCrossT = glm::cross(rRel, t);
            const float denomT = 1.0f / body->M + glm::dot(rCrossT, body->Iinv * rCrossT);
            if (!std::isfinite(denomT) || denomT <= 1e-8f) continue;
            float jT = -glm::dot(t, v) / denomT;
            const float maxFriction = mu * j;
            jT = std::clamp(jT, -maxFriction, maxFriction);
            if (!std::isfinite(jT)) continue;
            J += jT * t;
        }

        body->P += J;
        body->L += glm::cross(rRel, J);
    }
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

float safeExtent(float v) {
    return std::max(std::abs(v), 1e-3f);
}

BodySnapshot captureBodySnapshot(const RigidBox& body) {
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

void applyBodySnapshot(RigidBox& body, const BodySnapshot& snapshot) {
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
    const RigidBox& body,
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
    const RigidBox& body,
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
    RigidBox& body,
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

void physicsSolverSystem(Registry& registry, AppContext& ctx) {
    using namespace ecs::physics_detail;

    if (!ctx.animation) return;

    auto& rigidBodies = registry.storage<ecs::RigidBody>();
    auto& transforms = registry.storage<ecs::Transform>();
    auto& meshRefs = registry.storage<ecs::MeshRef>();
    auto& meshAssets = ctx.scene->getMeshAssets();
    
    auto& planes = registry.storage<ecs::Plane>();
    auto& planeColliders = registry.storage<ecs::PlaneCollider>();

    static std::unordered_map<ecs::Entity, SolverState> states;

    for (auto it = states.begin(); it != states.end();) {
        if (!rigidBodies.has(it->first) || !transforms.has(it->first) || !meshRefs.has(it->first)) {
            it = states.erase(it);
        } else {
            ++it;
        }
    }

    const int currFrame = ctx.animation->getFrame();
    const float dt = static_cast<float>(ctx.animation->getFixedDt());
    static int prevFrame = -1;
    const bool frameChanged = (currFrame != prevFrame);
    prevFrame = currFrame;

    bool changed = false;

    for (const ecs::Entity& e : rigidBodies.entities()) {
        if (!transforms.has(e) || !meshRefs.has(e)) continue;

        ecs::RigidBody& rb = rigidBodies.get(e);
        ecs::Transform& t = transforms.get(e);
        ecs::MeshRef& meshRef = meshRefs.get(e);
        if (meshRef.handle < 0 || static_cast<size_t>(meshRef.handle) >= meshAssets.size()) continue;

        auto it = states.find(e);
        const bool needsInit = (
            it == states.end() ||
            it->second.meshHandle != meshRef.handle ||
            !it->second.body
        );

        if (needsInit) {
            const MeshAsset& asset = meshAssets[meshRef.handle];

            glm::vec3 boundsMin(0.0f), boundsMax(0.0f);
            computeMeshBounds(asset, boundsMin, boundsMax);

            const glm::vec3 localCenter = 0.5f * (boundsMin + boundsMax);
            const glm::vec3 localHalfExtents = 0.5f * (boundsMax - boundsMin);

            glm::vec3 scaledHalfExtents = glm::abs(t.scale) * localHalfExtents;
            scaledHalfExtents.x = safeExtent(scaledHalfExtents.x);
            scaledHalfExtents.y = safeExtent(scaledHalfExtents.y);
            scaledHalfExtents.z = safeExtent(scaledHalfExtents.z);

            SolverState state;
            state.meshHandle = meshRef.handle;
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

            it = states.insert_or_assign(e, std::move(state)).first;
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

        int startFrame = state.initializedFrame;
        auto bestStart = state.snapshots.end();
        for (auto itSnap = state.snapshots.begin(); itSnap != state.snapshots.end(); ++itSnap) {
            if (itSnap->first < currFrame && (bestStart == state.snapshots.end() || itSnap->first > bestStart->first)) {
                bestStart = itSnap;
            }
        }

        if (bestStart == state.snapshots.end()) {
            state.initializedFrame = currFrame;
            state.snapshots.insert_or_assign(currFrame, captureFrameSnapshot(*state.body, t, rb));
            changed = true;
            continue;
        }

        startFrame = bestStart->first;
        applyFrameSnapshot(*state.body, bestStart->second, t, rb);

        for (int f = startFrame + 1; f <= currFrame; ++f) {
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

} // namespace ecs
