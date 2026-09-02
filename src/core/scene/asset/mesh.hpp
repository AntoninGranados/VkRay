#pragma once

#include <optional>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "core/scene/gpu_structs.hpp"

struct Vertex {
    alignas(16) glm::vec3 position;
    alignas(16) glm::vec3 normal;
    alignas(16) glm::vec3 color = glm::vec3(1.0f);
};

class MeshAsset {
public:
    MeshAsset() = default;
    MeshAsset(std::vector<Vertex> vertices, std::vector<uint32_t> indices);

    static std::optional<MeshAsset> load(const std::string& path);

    void buildBvh();
    float computeArea(const glm::mat4& transform) const;

    const std::vector<Vertex>& getVertices() const { return vertices; }
    const std::vector<uint32_t>& getIndices() const { return indices; }
    const std::vector<GpuBvhNode>& getBvhNodes() const { return bvhNodes; }
    glm::vec3 getAabbMin() const { return aabbMin; }
    glm::vec3 getAabbMax() const { return aabbMax; }
    bool hasVertexColor() const { return vertexColorLoaded; }

private:
    bool vertexColorLoaded = false;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<GpuBvhNode> bvhNodes;
    glm::vec3 aabbMin{0.0f};
    glm::vec3 aabbMax{0.0f};

    struct TriBounds {
        glm::vec3 min;
        glm::vec3 max;
        glm::vec3 centroid;
    };

    static constexpr int kLeafSize = 16;
    static constexpr int kSahK = 12;

    size_t buildBvhNode(std::vector<TriBounds>& triBounds, std::vector<uint32_t>& triIndices, uint32_t start, uint32_t count, glm::vec3& outAabbMin, glm::vec3& outAabbMax);
};

MeshAsset makeDefaultMeshAsset();
