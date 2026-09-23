#include "raycast.hpp"

#include "core/core.hpp"
#include "core/ecs/components/camera.hpp"
#include "core/ecs/components/component.hpp"
#include "core/ecs/components/core.hpp"
#include "core/ecs/entity.hpp"
#include "core/ecs/components.hpp"
#include "core/scene/asset/mesh.hpp"
#include "core/camera/camera.hpp"

Ray getRay(const glm::vec2& mousePos, const glm::vec2& screenSize, const ecs::Entity& camera) {
    const ecs::Registry& registry = Core::getScene().getRegistry();
    const ecs::Component& t = registry.get(camera, ecs::Transform);

    const float invWidth = 1.0f / screenSize.x;
    const float invHeight = 1.0f / screenSize.y;

    const glm::vec3 forward = directionFromRotation(t.get<glm::vec3>("rotation"));
    const glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
    const glm::vec3 up = glm::cross(right, forward);

    const float ndcX = mousePos.x * 2.0f * invWidth - 1.0f;
    const float ndcY = 1.0f - mousePos.y * 2.0f * invHeight;

    const float tanHFov = glm::tan(glm::radians(effectiveFov(registry, camera)) * 0.5f);
    const float aspect = screenSize.x * invHeight;

    const float camX = ndcX * aspect * tanHFov;
    const float camY = ndcY * tanHFov;

    glm::vec3 dir = glm::normalize(camX * right + camY * up + forward);
    return Ray{ t.get<glm::vec3>("position"), dir };
}

float raySphereIntersection(const Ray& ray, const glm::mat4& transform) {
    const glm::mat4 invTransform = glm::inverse(transform);
    const glm::vec3 localOrigin = glm::vec3(invTransform * glm::vec4(ray.origin, 1.0f));
    const glm::vec3 localDir = glm::vec3(invTransform * glm::vec4(ray.dir, 0.0f));

    const glm::vec3 p = -localOrigin;
    const float a = glm::dot(localDir, localDir);
    const float b = glm::dot(localDir, p);
    const float c = glm::dot(p, p) - 1.0f;
    const float delta = b * b - a * c;
    if (delta < 0.0f) return -1.0f;

    const float sqrtDelta = std::sqrt(delta);
    float tLocal = (b - sqrtDelta) / a;
    if (tLocal < 0.0f) tLocal = (b + sqrtDelta) / a;
    if (tLocal < 0.0f) return -1.0f;

    const glm::vec3 hitLocal = localOrigin + localDir * tLocal;
    const glm::vec3 hitWorld = glm::vec3(transform * glm::vec4(hitLocal, 1.0f));
    return glm::dot(hitWorld - ray.origin, ray.dir);
}

float rayPlaneIntersection(const Ray& ray, const glm::mat4& transform) {
    const glm::mat4 invTransform = glm::inverse(transform);
    const glm::vec3 localOrigin = glm::vec3(invTransform * glm::vec4(ray.origin, 1.0f));
    const glm::vec3 localDir = glm::vec3(invTransform * glm::vec4(ray.dir, 0.0f));

    if (std::abs(localDir.y) <= 1e-12f) return -1.0f;
    const float tLocal = -localOrigin.y / localDir.y;
    if (tLocal < 0.0f) return -1.0f;

    const glm::vec3 hitLocal = localOrigin + localDir * tLocal;
    const glm::vec3 hitWorld = glm::vec3(transform * glm::vec4(hitLocal, 1.0f));
    return glm::dot(hitWorld - ray.origin, ray.dir);
}

float rayBoxIntersection(const Ray& ray, const glm::mat4& transform) {
    glm::mat4 invTransform = glm::inverse(transform);
    glm::vec3 localOrigin = glm::vec3(invTransform * glm::vec4(ray.origin, 1.0f));
    glm::vec3 localDir = glm::vec3(invTransform * glm::vec4(ray.dir, 0.0f));
    Ray localRay{ localOrigin, localDir };

    float tmin = -std::numeric_limits<float>::infinity();
    float tmax = std::numeric_limits<float>::infinity();

    for (int i = 0; i < 3; ++i) {
        const float dir = localRay.dir[i];
        if (std::abs(dir) < 1e-8f) {
            if (localRay.origin[i] < -1.0f || localRay.origin[i] > 1.0f) return -1.0f;
            continue;
        }

        const float invD = 1.0f / dir;
        float t0 = (-1.0f - localRay.origin[i]) * invD;
        float t1 = ( 1.0f - localRay.origin[i]) * invD;
        if (invD < 0.0f) std::swap(t0, t1);

        if (t0 > tmin) tmin = t0;
        if (t1 < tmax) tmax = t1;
        if (tmax < tmin) return -1.0f;
    }

    if (tmax < 0.0f) return -1.0f;
    return tmin >= 0.0f ? tmin : tmax;
}

float rayQuadIntersection(const Ray& ray, const glm::mat4& transform) {
    const glm::mat4 invTransform = glm::inverse(transform);
    const glm::vec3 localOrigin = glm::vec3(invTransform * glm::vec4(ray.origin, 1.0f));
    const glm::vec3 localDir = glm::vec3(invTransform * glm::vec4(ray.dir, 0.0f));

    if (localDir.z >= -1e-12f) return -1.0f; // back-face or parallel
    const float tLocal = -localOrigin.z / localDir.z;
    if (tLocal < 0.0f) return -1.0f;

    const glm::vec3 hitLocal = localOrigin + localDir * tLocal;
    if (std::abs(hitLocal.x) > 0.5f || std::abs(hitLocal.y) > 0.5f) return -1.0f;

    const glm::vec3 hitWorld = glm::vec3(transform * glm::vec4(hitLocal, 1.0f));
    return glm::dot(hitWorld - ray.origin, ray.dir);
}

static bool rayTriangleIntersection(const glm::vec3& origin, const glm::vec3& dir,
                                    const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
                                    float& tOut) {
    const float epsilon = 1e-6f;
    const glm::vec3 edge1 = v1 - v0;
    const glm::vec3 edge2 = v2 - v0;
    const glm::vec3 pvec = glm::cross(dir, edge2);
    const float det = glm::dot(edge1, pvec);
    if (fabs(det) < epsilon) return false;
    const float invDet = 1.0f / det;
    const glm::vec3 tvec = origin - v0;
    const float u = glm::dot(tvec, pvec) * invDet;
    if (u < 0.0f || u > 1.0f) return false;
    const glm::vec3 qvec = glm::cross(tvec, edge1);
    const float v = glm::dot(dir, qvec) * invDet;
    if (v < 0.0f || u + v > 1.0f) return false;
    const float hitT = glm::dot(edge2, qvec) * invDet;
    if (hitT < 0.0f) return false;
    tOut = hitT;
    return true;
}

float rayMeshIntersection(const Ray& ray, const glm::mat4& transform,
                          const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) {
    if (indices.empty() || vertices.empty())
        return -1.0f;

    const glm::mat4 invTransform = glm::inverse(transform);
    const glm::vec3 localOrigin = glm::vec3(invTransform * glm::vec4(ray.origin, 1.0f));
    const glm::vec3 localDir = glm::vec3(invTransform * glm::vec4(ray.dir, 0.0f));

    float closest = std::numeric_limits<float>::infinity();
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        const uint32_t i0 = indices[i + 0];
        const uint32_t i1 = indices[i + 1];
        const uint32_t i2 = indices[i + 2];
        if (i0 >= vertices.size() || i1 >= vertices.size() || i2 >= vertices.size())
            continue;

        float tLocal = 0.0f;
        if (!rayTriangleIntersection(localOrigin, localDir,
                                     vertices[i0].position,
                                     vertices[i1].position,
                                     vertices[i2].position,
                                     tLocal))
            continue;

        const glm::vec3 hitLocal = localOrigin + localDir * tLocal;
        const glm::vec3 hitWorld = glm::vec3(transform * glm::vec4(hitLocal, 1.0f));
        const float tWorld = glm::dot(hitWorld - ray.origin, ray.dir);
        if (tWorld >= 0.0f && tWorld < closest)
            closest = tWorld;
    }

    return std::isfinite(closest) ? closest : -1.0f;
}
