#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "core/ecs/entity.hpp"

struct Vertex;

struct Ray {
    glm::vec3 origin;
    glm::vec3 dir;
};

Ray getRay(const glm::vec2& mousePos, const glm::vec2& screenSize, const ecs::Entity& camera);

float raySphereIntersection(const Ray& ray, const glm::mat4& transform);
float rayPlaneIntersection(const Ray& ray, const glm::mat4& transform);
float rayBoxIntersection(const Ray& ray, const glm::mat4& transform);
float rayQuadIntersection(const Ray& ray, const glm::mat4& transform);
float rayMeshIntersection(const Ray& ray, const glm::mat4& transform, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
