#include "objects.hpp"

#include <algorithm>
#include <cmath>
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

float rayPlaneIntersection(const Ray &ray, const Plane &plane) {
    const float denom = glm::dot(plane.normal, ray.dir);
    if (std::abs(denom) <= 1e-6f) return -1.0f;

    const float t = glm::dot(plane.point - ray.origin, plane.normal) / denom;
    return t >= 0.0f ? t : -1.0f;
}

float rayBoxIntersection(const Ray &ray, const Box &box) {
    float tmin = 0.0f;
    float tmax = std::numeric_limits<float>::infinity();

    for (int i = 0; i < 3; ++i) {
        const float dir = ray.dir[i];
        if (std::abs(dir) < 1e-8f) {
            if (ray.origin[i] < box.cornerMin[i] || ray.origin[i] > box.cornerMax[i]) return -1.0f;
            continue;
        }

        const float invD = 1.0f / dir;
        float t0 = (box.cornerMin[i] - ray.origin[i]) * invD;
        float t1 = (box.cornerMax[i] - ray.origin[i]) * invD;
        if (invD < 0.0f) std::swap(t0, t1);

        if (t0 > tmin) tmin = t0;
        if (t1 < tmax) tmax = t1;
        if (tmax < tmin) return -1.0f;
    }

    if (tmin >= 0.0f) return tmin;
    return tmax >= 0.0f ? tmax : -1.0f;
}

float rayObjectIntersection(
    const Ray &ray,
    const Object &obj,
    const std::vector<Sphere> &spheres,
    const std::vector<Plane> &planes,
    const std::vector<Box> &boxes
) {
    switch (obj.type) {
        case SPHERE:
            return raySphereIntersection(ray, spheres[obj.id]);
        case PLANE:
            return rayPlaneIntersection(ray, planes[obj.id]);
        case BOX:
            return rayBoxIntersection(ray, boxes[obj.id]);
        default: break;
    }
    return -1.0f;
}

bool closestObjectHit(
    const Ray &ray,
    const std::vector<Object> &objects,
    const std::vector<Sphere> &spheres,
    const std::vector<Plane> &planes,
    const std::vector<Box> &boxes,
    Hit &hit
) {
    float closest_t = std::numeric_limits<float>::infinity();
    int closest_idx = -1;

    for (size_t i = 0; i < objects.size(); ++i) {
        const float t = rayObjectIntersection(ray, objects[i], spheres, planes, boxes);
        if (t >= 0.0f && t < closest_t) {
            closest_t = t;
            closest_idx = static_cast<int>(i);
        }
    }

    if (closest_idx < 0) return false;

    hit = {
        .p = ray.origin + closest_t * ray.dir,
        .t = closest_t,
        .idx = closest_idx,
    };
    return true;
}
