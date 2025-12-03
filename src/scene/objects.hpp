#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "../camera.hpp"

enum MaterialType {
    lambertian = 0,
    metal,
    dielectric,
    emissive,
    animated,
};

struct Material {
    MaterialType type;
    alignas(16) glm::vec3 albedo;
    float fuzz;
    float refraction_index;
    float intensity;
};

// Objects
enum ObjectType {
    NONE,
    SPHERE,
    PLANE,
    BOX,
};

struct Sphere {
    alignas(16) glm::vec3 center;
    float radius;
    Material mat;
};

//? NOT USUED FOR NOW
struct Plane {
    alignas(16) glm::vec3 point;
    alignas(16) glm::vec3 normal;
    Material mat;
};

//? NOT USUED FOR NOW
struct Box {
    alignas(16) glm::vec3 cornerMin;
    alignas(16) glm::vec3 cornerMax;
    Material mat;
};

struct Object {
    ObjectType type;
    int id;
};

// Raytracing
struct Ray {
    glm::vec3 origin;
    glm::vec3 dir;
};

struct Hit {
    glm::vec3 p;
    float t;
    int idx;
};

Ray getRay(const glm::vec2 &mousePos, const glm::vec2 &screenSize, const Camera &camera);

float raySphereIntersection(const Ray &ray, const Sphere &sphere);
float rayPlaneIntersection(const Ray &ray, const Plane &plane);
float rayBoxIntersection(const Ray &ray, const Box &box);
float rayObjectIntersection(
    const Ray &ray,
    const Object &obj,
    const std::vector<Sphere> &spheres,
    const std::vector<Plane> &planes,
    const std::vector<Box> &boxes
);
bool closestObjectHit(
    const Ray &ray,
    const std::vector<Object> &objects,
    const std::vector<Sphere> &spheres,
    const std::vector<Plane> &planes,
    const std::vector<Box> &boxes,
    Hit &hit
);
