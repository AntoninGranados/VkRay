#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "object.hpp"
#include "material.hpp"
#include "imgui/imgui.h"
#include "imgui/ImGuizmo.h"

#define LEAF_SIZE 4

struct GpuBvhNode {
    alignas(16) glm::vec3 aabbMin;
    alignas(16) glm::vec3 aabbMax;
    uint32_t data0; // left or first triangle
    uint32_t data1; // right or triangle count
    uint32_t isLeaf;
};

#define BVH_childLeft(node)   (node.data0)
#define BVH_childRight(node)  (node.data1)
#define BVH_firstTriangle(node) (node.data0)
#define BVH_triangleCount(node) (node.data1)

struct GpuMesh {
    alignas(16) glm::mat4 transform;
    alignas(16) glm::mat4 invTransform;
    uint32_t indexOffset;
    uint32_t triangleCount;
    uint32_t bvhOffset;
    uint32_t bvhNodeCount;
    MaterialHandle materialHandle;
};

struct Vertex {
    alignas(16) glm::vec3 position;
};

class Mesh: public Object {
public:
    Mesh(std::string name, std::vector<Vertex> vertices, std::vector<uint32_t> indices, glm::mat4 transform, MaterialHandle materialHandle);
    float rayIntersection(const Ray &ray) override;
    bool drawGuizmo(const glm::mat4 &view, const glm::mat4 &proj) override;
    bool drawUI(std::vector<Material> &materials) override;
    
    float getArea() override;
    GpuMesh getStruct();
    const std::vector<Vertex>& getVertices() const { return vertices; }
    const std::vector<uint32_t>& getIndices() const { return indices; }
    const std::vector<GpuBvhNode>& getBvhNodes() const { return bvhNodes; }
    const glm::mat4 getTransform() const { return transform; }
    ObjectType getType() override { return ObjectType::Mesh; };

private:
    GpuMesh mesh;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<GpuBvhNode> bvhNodes;
    glm::mat4 transform;
    MaterialHandle materialHandle;

    struct TriBounds {
        glm::vec3 min;
        glm::vec3 max;
        glm::vec3 centroid;
    };
    
    size_t buildBvhNode(std::vector<TriBounds> &triBounds, std::vector<uint32_t> &triIndices, uint32_t start, uint32_t count);
    void buildBvh();
};
