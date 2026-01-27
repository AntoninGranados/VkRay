#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <string>

#include "../object/object.hpp"
#include "../../app_context.hpp"

typedef int MeshHandle;

#define LEAF_SIZE 4

struct Vertex {
    alignas(16) glm::vec3 position;
};

class MeshAsset {
public:
    MeshAsset(const std::string& name): name(name) {};
    bool loadFromObj(const AppContext& ctx, const std::string& path);
    void buildBvh();
    const std::vector<Vertex>& getVertices() const { return vertices; }
    const std::vector<uint32_t>& getIndices() const { return indices; }
    const std::vector<GpuBvhNode>& getBvhNodes() const { return bvhNodes; }
    const std::string& getName() const { return name; }
    const std::string& getPath() const { return path; }

private:
    std::string name;
    std::string path;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<GpuBvhNode> bvhNodes;

    struct TriBounds {
        glm::vec3 min;
        glm::vec3 max;
        glm::vec3 centroid;
    };

    size_t buildBvhNode(std::vector<TriBounds> &triBounds, std::vector<uint32_t> &triIndices, uint32_t start, uint32_t count);
};
