#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "core/scene/object/object.hpp"

typedef int MeshHandle;

static constexpr int LEAF_SIZE = 16;
static constexpr int SAH_K     = 12;

struct Vertex {
    alignas(16) glm::vec3 position;
    alignas(16) glm::vec3 normal;
    alignas(16) glm::vec3 color = glm::vec3(1.0f);
};


class MeshAsset {
public:
    MeshAsset(const std::string& name): name(name) {};
    MeshAsset(const std::string& name, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

    bool loadFromObj(const std::string& path);
    void buildBvh();
    const std::vector<Vertex>& getVertices() const { return vertices; }
    const std::vector<uint32_t>& getIndices() const { return indices; }
    const std::vector<GpuBvhNode>& getBvhNodes() const { return bvhNodes; }
    glm::vec3 getAabbMin() const { return aabbMin; }
    glm::vec3 getAabbMax() const { return aabbMax; }
    const std::string& getName() const { return name; }
    void setName(const std::string& newName) { name = newName; }
    const std::string& getPath() const { return path; }
    void setPath(const std::string& newPath) { path = newPath; }
    static std::string nameFromPath(const std::string& path);
    
    float computeArea(const glm::mat4& transform) const;

    float getSimplifyRatio() const { return simplifyRatio; }
    bool setSimplifyRatio(float ratio);
    bool applySimplification();
    bool revertSimplification();

    bool getSmoothShading() const { return smoothShading; }
    void setSmoothShading(bool v) { smoothShading = v; }
    bool hasVertexColor() const { return vertexColorLoaded; }

private:
    std::string name;
    std::string path = "";
    float simplifyRatio = 1.0f;
    bool smoothShading     = false;
    bool vertexColorLoaded = false;
    bool hasSaved = false;
    std::vector<Vertex> savedVertices;
    std::vector<uint32_t> savedIndices;
    std::vector<Vertex> baseVertices;
    std::vector<uint32_t> baseIndices;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<GpuBvhNode> bvhNodes;
    glm::vec3 aabbMin;
    glm::vec3 aabbMax;

    struct TriBounds {
        glm::vec3 min;
        glm::vec3 max;
        glm::vec3 centroid;
    };

    size_t buildBvhNode(std::vector<TriBounds>& triBounds, std::vector<uint32_t>& triIndices, uint32_t start, uint32_t count, glm::vec3& outAabbMin, glm::vec3& outAabbMax);
};

bool drawMeshAssetUI(MeshAsset& mesh);

inline MeshAsset makeDefaultMeshAsset() {
    return MeshAsset("Cube",
        std::vector<Vertex>{
            { glm::vec3(-0.5f, -0.5f, -0.5f), glm::normalize(glm::vec3(-0.5f, -0.5f, -0.5f)) },
            { glm::vec3( 0.5f, -0.5f, -0.5f), glm::normalize(glm::vec3( 0.5f, -0.5f, -0.5f)) },
            { glm::vec3( 0.5f,  0.5f, -0.5f), glm::normalize(glm::vec3( 0.5f,  0.5f, -0.5f)) },
            { glm::vec3(-0.5f,  0.5f, -0.5f), glm::normalize(glm::vec3(-0.5f,  0.5f, -0.5f)) },
            { glm::vec3(-0.5f, -0.5f,  0.5f), glm::normalize(glm::vec3(-0.5f, -0.5f,  0.5f)) },
            { glm::vec3( 0.5f, -0.5f,  0.5f), glm::normalize(glm::vec3( 0.5f, -0.5f,  0.5f)) },
            { glm::vec3( 0.5f,  0.5f,  0.5f), glm::normalize(glm::vec3( 0.5f,  0.5f,  0.5f)) },
            { glm::vec3(-0.5f,  0.5f,  0.5f), glm::normalize(glm::vec3(-0.5f,  0.5f,  0.5f)) },
        },
        std::vector<uint32_t>{
            0, 1, 2, 2, 3, 0,
            4, 5, 6, 6, 7, 4,
            1, 5, 6, 6, 2, 1,
            0, 3, 7, 7, 4, 0,
            3, 2, 6, 6, 7, 3,
            0, 4, 5, 5, 1, 0,
        }
    );
}
