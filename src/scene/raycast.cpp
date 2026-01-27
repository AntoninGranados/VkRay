#include "raycast.hpp"

#include "../camera.hpp"

Ray getRay(const glm::vec2 &mousePos, const glm::vec2 &screenSize, const Camera &camera) {
    const float invWidth = 1.0f / screenSize.x;
    const float invHeight = 1.0f / screenSize.y;

    const glm::vec3 forward = glm::normalize(camera.getDirection());
    const glm::vec3 right = glm::normalize(glm::cross(forward, camera.getUp()));
    const glm::vec3 up = glm::cross(right, forward);

    const float ndcX = mousePos.x * 2.0f * invWidth - 1.0f;
    const float ndcY = 1.0f - mousePos.y * 2.0f * invHeight;

    const float tanHFov = camera.getTanHFov();
    const float aspect = screenSize.x * invHeight;

    const float camX = ndcX * aspect * tanHFov;
    const float camY = ndcY * tanHFov;

    glm::vec3 dir = glm::normalize(camX * right + camY * up + forward);
    return Ray{ camera.getPosition(), dir };
}

float raySphereIntersection(const Ray &ray, const glm::vec3& center, const float& radius) {
    const glm::vec3 p = center - ray.origin;
    const float dp = glm::dot(ray.dir, p);
    const float c = glm::dot(p, p) - radius * radius;
    const float delta = dp * dp - c;
    if (delta < 0.0f) return -1.0f;

    const float sqrt_delta = std::sqrt(delta);

    float t = dp - sqrt_delta;
    if (t >= 0.0f) return t;

    t = dp + sqrt_delta;
    return t >= 0.0f ? t : -1.0f;
}

float rayPlaneIntersection(const Ray &ray, const glm::vec3& point, const glm::vec3& normal) {
    const float denom = glm::dot(normal, ray.dir);
    if (std::abs(denom) <= 1e-12f) return -1.0f;

    const float t = glm::dot(point - ray.origin, normal) / denom;
    return t >= 0.0f ? t : -1.0f;
}

float rayBoxIntersection(const Ray &ray, const glm::mat4& transform) {
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