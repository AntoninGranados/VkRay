#include "objects.hpp"

#include <limits>

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

float raySphereIntersection(const Ray &ray, const Sphere &sphere) {
    const glm::vec3 p = sphere.center - ray.origin;
    const float dp = glm::dot(ray.dir, p);
    const float c = glm::dot(p, p) - sphere.radius * sphere.radius;
    const float delta = dp * dp - c;
    if (delta < 0.0f) return -1.0f;

    const float sqrt_delta = std::sqrt(delta);

    float t = dp - sqrt_delta;
    if (t >= 0.0f) return t;

    t = dp + sqrt_delta;
    return t >= 0.0f ? t : -1.0f;
}

bool closestSphereHit(const Ray &ray, const std::vector<Sphere> &spheres, Hit &hit) {
    float closest_t = std::numeric_limits<float>::infinity();
    int closest_idx = -1;

    for (size_t i = 0; i < spheres.size(); ++i) {
        const float t = raySphereIntersection(ray, spheres[i]);
        if (t >= 0.0f && t < closest_t) {
            closest_t = t;
            closest_idx = static_cast<int>(i);
        }
    }

    if (closest_idx < 0) return false;

    const glm::vec3 p = ray.origin + closest_t * ray.dir;

    hit = {
        .p = p,
        .t = closest_t,
        .idx = closest_idx,
    };
    return true;
}
