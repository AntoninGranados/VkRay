#include "physics_solver.hpp"

#include <algorithm>
#include <cmath>

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

RigidSphere::RigidSphere(
    const float r, const float dens,
    const glm::vec3 v0, const glm::vec3 omega0) : radius(r)
{
    V = v0;
    omega = omega0;

    const float rSafe = std::max(radius, 1e-4f);
    M = dens * (4.0f / 3.0f) * glm::pi<float>() * rSafe * rSafe * rSafe;
    const float i = (2.0f / 5.0f) * M * rSafe * rSafe;
    I0 = glm::mat3(i, 0, 0, 0, i, 0, 0, 0, i);
    I0inv = glm::inverse(I0);
    Iinv = R * I0inv * glm::transpose(R);

    // Sampled support points for contacts (Fibonacci sphere)
    const int kSphereSamples = 256;
    const float kGoldenAngle = glm::pi<float>() * (3.0f - std::sqrt(5.0f));
    vdata0.reserve(kSphereSamples);
    for (int i = 0; i < kSphereSamples; ++i) {
        const float y = 1.0f - 2.0f * (static_cast<float>(i) + 0.5f) / static_cast<float>(kSphereSamples);
        const float r = std::sqrt(std::max(0.0f, 1.0f - y * y));
        const float theta = kGoldenAngle * static_cast<float>(i);
        const float x = std::cos(theta) * r;
        const float z = std::sin(theta) * r;
        vdata0.emplace_back(x * rSafe, y * rSafe, z * rSafe);
    }
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

void RigidSolver::step(const Entity bodyEntity, const float dt, Registry& registry) {
    if (!body || dt <= 0.0f) return;
    appTime += dt;
    const float colliderDt = std::max(dt, 1e-8f);

    auto& colliders = registry.storage<Collider>();

    while (appTime > simTime) {
        computeForceAndTorque();
        integrate(simDt * 0.5f);
        for (const Entity& e : colliders.entities()) {
            if (e == bodyEntity) continue;
            if (registry.storage<Plane>().has(e)) resolvePlaneCollision(e, registry, colliderDt);
            if (registry.storage<Sphere>().has(e)) resolveSphereCollision(e, registry, colliderDt);
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

glm::vec3 computeColliderPointVelocity(
    const Transform& t,
    const Transform& prevTransform,
    const glm::vec3& point,
    const glm::vec3& center,
    const float colliderDt
) {
    const glm::vec3 linearVelocity = (t.position - prevTransform.position) / colliderDt;
    const glm::quat qPrev = glm::normalize(prevTransform.rotation);
    const glm::quat qCurr = glm::normalize(t.rotation);
    glm::quat dq = qCurr * glm::inverse(qPrev);
    if (dq.w < 0.0f) dq = -dq;
    const float w = std::clamp(dq.w, -1.0f, 1.0f);
    const float angle = 2.0f * std::acos(w);
    const float sinHalf = std::sqrt(std::max(0.0f, 1.0f - w * w));
    glm::vec3 angularVelocity(0.0f);
    if (sinHalf > 1e-6f && std::isfinite(angle)) {
        const glm::vec3 axis = glm::vec3(dq.x, dq.y, dq.z) / sinHalf;
        angularVelocity = axis * (angle / colliderDt);
    }

    return linearVelocity + glm::cross(angularVelocity, point - center);
}

void RigidSolver::resolveSdfCollision(
    const Entity& e,
    Registry& registry,
    const float colliderDt,
    const SdfSampler& sdfSampler
) {
    auto& colliders = registry.storage<Collider>();
    auto& transforms = registry.storage<Transform>();
    if (!colliders.has(e) || !transforms.has(e)) return;

    const Collider& collider = colliders.get(e);
    const Transform& t = transforms.get(e);
    const float eps = collider.restitution;
    const float mu = collider.friction;
    Transform prevTransform {};
    const bool hasPrev = getPrevColliderTransform(e, prevTransform);

    for (size_t i = 0; i < body->vdata0.size(); i++) {
        const glm::vec3 rRel = body->R * body->vdata0[i];
        const glm::vec3 r = rRel + body->X;

        SdfContactSample sample {};
        if (!sdfSampler(r, sample)) continue;
        const float dist = std::abs(sample.distance);
        const glm::vec3 normal = (sample.distance >= 0.0f) ? sample.normal : -sample.normal;

        glm::vec3 colliderVelocity(0.0f);
        if (hasPrev) {
            colliderVelocity = computeColliderPointVelocity(t, prevTransform, r, sample.center, colliderDt);
        }

        const glm::vec3 v = body->V + glm::cross(body->omega, rRel) - colliderVelocity;
        const float vRel = glm::dot(normal, v);
        if (vRel >= 1e-1f) continue;

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
            const glm::vec3 tDir = vT / vTlen;
            const glm::vec3 rCrossT = glm::cross(rRel, tDir);
            const float denomT = 1.0f / body->M + glm::dot(rCrossT, body->Iinv * rCrossT);
            if (!std::isfinite(denomT) || denomT <= 1e-8f) continue;
            float jT = -glm::dot(tDir, v) / denomT;
            const float maxFriction = mu * j;
            jT = std::clamp(jT, -maxFriction, maxFriction);
            if (!std::isfinite(jT)) continue;
            J += jT * tDir;
        }

        body->P += J;
        body->L += glm::cross(rRel, J);
    }
}

void RigidSolver::resolvePlaneCollision(const Entity& e, Registry& registry, const float colliderDt) {
    auto& planes = registry.storage<Plane>();
    auto& transforms = registry.storage<Transform>();
    if (!planes.has(e) || !transforms.has(e)) return;

    const Transform& t = transforms.get(e);
    const glm::vec3 p0 = t.position;
    const glm::vec3 n = t.rotation * glm::vec3(0.0f, 1.0f, 0.0f);
    const SdfSampler sdfSampler = [p0, n](const glm::vec3& p, SdfContactSample& sample) -> bool {
        sample.distance = glm::dot(n, p - p0);
        sample.normal = n;
        sample.center = p0;
        return true;
    };
    resolveSdfCollision(e, registry, colliderDt, sdfSampler);
}

void RigidSolver::resolveSphereCollision(const Entity& e, Registry& registry, const float colliderDt) {
    auto& spheres = registry.storage<Sphere>();
    auto& transforms = registry.storage<Transform>();
    if (!spheres.has(e) || !transforms.has(e)) return;

    const Sphere& sphere = spheres.get(e);
    const Transform& t = transforms.get(e);
    const glm::vec3 center = t.position;
    const float maxScale = std::max(std::max(std::abs(t.scale.x), std::abs(t.scale.y)), std::abs(t.scale.z));
    const float radius = std::max(1e-4f, sphere.radius * maxScale);
    const SdfSampler sdfSampler = [center, radius](const glm::vec3& p, SdfContactSample& sample) -> bool {
        const glm::vec3 delta = p - center;
        const float deltaLen = glm::length(delta);
        if (!std::isfinite(deltaLen) || deltaLen <= 1e-8f) return false;
        sample.distance = deltaLen - radius;
        sample.normal = delta / deltaLen;
        sample.center = center;
        return true;
    };
    resolveSdfCollision(e, registry, colliderDt, sdfSampler);
}

} // namespace physics_detail

} // namespace ecs
