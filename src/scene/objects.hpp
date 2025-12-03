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

struct Sphere {
    alignas(16) glm::vec3 center;
    float radius;
    Material mat;
};



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
bool closestSphereHit(const Ray &ray, const std::vector<Sphere> &spheres, Hit &hit);
