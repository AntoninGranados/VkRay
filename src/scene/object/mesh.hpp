#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "object.hpp"
#include "material.hpp"
#include "imgui/imgui.h"
#include "imgui/ImGuizmo.h"


struct GpuMesh {
    alignas(16) glm::mat4 transform;
    alignas(16) glm::mat4 invTransform;
    alignas(16) glm::vec3 aabbMin;
    alignas(16) glm::vec3 aabbMax;
    unsigned int indexOffset;
    unsigned int triangleCount;
    MaterialHandle materialHandle;
};

struct Vertex {
    alignas(16) glm::vec3 position;
};

class Mesh: public Object {
public:
    Mesh(std::string name, std::vector<Vertex> vertices, std::vector<unsigned int> indices, glm::mat4 transform, MaterialHandle materialHandle);
    float rayIntersection(const Ray &ray) override;
    bool drawGuizmo(const glm::mat4 &view, const glm::mat4 &proj) override;
    bool drawUI(std::vector<Material> &materials) override;
    
    float getArea() override;
    GpuMesh getStruct();
    const std::vector<Vertex>& getVertices() const { return vertices; }
    const std::vector<unsigned int>& getIndices() const { return indices; }
    const glm::mat4 getTransform() const { return transform; }
    ObjectType getType() override { return ObjectType::Mesh; };

private:
    GpuMesh mesh;

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    glm::mat4 transform;
    glm::vec3 aabbMin;
    glm::vec3 aabbMax;
    MaterialHandle materialHandle;
};
