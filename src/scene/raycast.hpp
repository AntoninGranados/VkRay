#pragma once

#include <vector>

#include <glm/glm.hpp>

struct Vertex;

class Camera;

struct Ray {
    glm::vec3 origin;
    glm::vec3 dir;
};

Ray getRay(const glm::vec2& mousePos, const glm::vec2& screenSize, const Camera& camera);

float raySphereIntersection(const Ray& ray, const glm::vec3& center, const float& radius);
float rayPlaneIntersection(const Ray& ray, const glm::vec3& point, const glm::vec3& normal);
float rayBoxIntersection(const Ray& ray, const glm::mat4& transform);
float rayQuadIntersection(const Ray& ray, const glm::vec3& origin, const glm::vec3& u, const glm::vec3& v, const glm::vec3& normal);
float rayMeshIntersection(const Ray& ray, const glm::mat4& transform, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
