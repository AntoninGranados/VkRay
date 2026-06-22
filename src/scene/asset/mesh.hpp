#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <string>

#include "../object/object.hpp"
#include "app/app_context.hpp"

typedef int MeshHandle;

#define LEAF_SIZE 32

struct Vertex {
    alignas(16) glm::vec3 position;
    alignas(16) glm::vec3 normal;
};

#define DEFAULT_MESH_ASSET MeshAsset( \
    "Cube", \
    std::vector<Vertex>{ \
        { glm::vec3(-0.5f, -0.5f, -0.5f), glm::normalize(glm::vec3(-0.5f, -0.5f, -0.5f)) }, \
        { glm::vec3( 0.5f, -0.5f, -0.5f), glm::normalize(glm::vec3( 0.5f, -0.5f, -0.5f)) }, \
        { glm::vec3( 0.5f,  0.5f, -0.5f), glm::normalize(glm::vec3( 0.5f,  0.5f, -0.5f)) }, \
        { glm::vec3(-0.5f,  0.5f, -0.5f), glm::normalize(glm::vec3(-0.5f,  0.5f, -0.5f)) }, \
        { glm::vec3(-0.5f, -0.5f,  0.5f), glm::normalize(glm::vec3(-0.5f, -0.5f,  0.5f)) }, \
        { glm::vec3( 0.5f, -0.5f,  0.5f), glm::normalize(glm::vec3( 0.5f, -0.5f,  0.5f)) }, \
        { glm::vec3( 0.5f,  0.5f,  0.5f), glm::normalize(glm::vec3( 0.5f,  0.5f,  0.5f)) }, \
        { glm::vec3(-0.5f,  0.5f,  0.5f), glm::normalize(glm::vec3(-0.5f,  0.5f,  0.5f)) }, \
    }, \
    std::vector<uint32_t>{ \
        0, 1, 2, 2, 3, 0, \
        4, 5, 6, 6, 7, 4, \
        1, 5, 6, 6, 2, 1, \
        0, 3, 7, 7, 4, 0, \
        3, 2, 6, 6, 7, 3, \
        0, 4, 5, 5, 1, 0, \
    } \
)

class MeshAsset {
public:
    MeshAsset(const std::string& name): name(name) {};
    MeshAsset(const std::string& name, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

    bool loadFromObj(const AppContext& ctx, const std::string& path);
    void buildBvh();
    const std::vector<Vertex>& getVertices() const { return vertices; }
    const std::vector<uint32_t>& getIndices() const { return indices; }
    const std::vector<GpuBvhNode>& getBvhNodes() const { return bvhNodes; }
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

private:
    std::string name;
    std::string path = "";
    float simplifyRatio = 1.0f;
    bool hasSaved = false;
    std::vector<Vertex> savedVertices;
    std::vector<uint32_t> savedIndices;
    std::vector<Vertex> baseVertices;
    std::vector<uint32_t> baseIndices;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<GpuBvhNode> bvhNodes;

    struct TriBounds {
        glm::vec3 min;
        glm::vec3 max;
        glm::vec3 centroid;
    };

    size_t buildBvhNode(std::vector<TriBounds>& triBounds, std::vector<uint32_t>& triIndices, uint32_t start, uint32_t count);
};

bool drawMeshAssetUI(MeshAsset& mesh);
