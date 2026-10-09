#pragma once

#include <cstddef>
#include <cstdint>
#include <queue>
#include <unordered_set>
#include <vector>

#include <glm/glm.hpp>

#include "mesh.hpp"

class MeshSimplifier {
public:
    explicit MeshSimplifier(const MeshAsset& input);

    MeshAsset simplify(float targetRatio);

private:
    struct Face {
        uint32_t v[3];
        uint32_t attribute[3];
        bool alive = true;
    };

    struct Edge {
        uint32_t v0;
        uint32_t v1;
        glm::vec3 position;
        float cost;
        uint32_t v0Version;
        uint32_t v1Version;
    };

    struct EdgeCompare {
        bool operator()(const Edge& a, const Edge& b) const { return a.cost > b.cost; }
    };

    struct EdgeFaces {
        uint32_t count = 0;
        uint32_t faces[2] = {};
    };

    struct PositionHash {
        size_t operator()(const glm::vec3& position) const;
    };

    static constexpr double kConstraintWeight = 1000.0;
    static constexpr float kTopologyFallbackEpsilon = 1e-12f;
    static constexpr float kFlipTolerance = 0.01f;

    static glm::dmat4 planeQuadric(const glm::dvec3& p0, const glm::dvec3& p1, const glm::dvec3& p2);
    static glm::dmat4 constraintQuadric(const glm::dvec3& p0, const glm::dvec3& p1, const glm::dvec3& opposite);
    static double quadricError(const glm::dmat4& q, const glm::dvec3& v);
    static bool solveOptimalPosition(const glm::dmat4& q, glm::dvec3& out);
    static int cornerOf(const Face& face, uint32_t v);
    static std::vector<glm::vec3> accumulateNormals(const std::vector<Vertex>& vertices,
                                                    const std::vector<uint32_t>& indices);

    void weldVertices();
    void buildAdjacency();
    void constrainEdges();
    void buildHeap();

    bool isDegenerateFace(const Face& face) const;
    bool colorsDiffer(const Face& a, const Face& b, uint32_t v) const;
    EdgeFaces findEdgeFaces(uint32_t v0, uint32_t v1) const;
    bool isSeam(const EdgeFaces& edgeFaces, uint32_t v0, uint32_t v1) const;
    float topologyCost(uint32_t v0, uint32_t v1) const;
    Edge computeEdge(uint32_t v0, uint32_t v1) const;
    bool flipsFaces(uint32_t moved, uint32_t other, const glm::vec3& position) const;
    bool canCollapse(const Edge& edge, const EdgeFaces& edgeFaces) const;
    void collapse(const Edge& edge, const EdgeFaces& edgeFaces);
    MeshAsset buildMesh() const;

    const std::vector<Vertex>& vertices;
    const std::vector<uint32_t>& indices;

    std::vector<glm::vec3> positions;
    std::vector<Face> faces;
    size_t liveTris = 0;

    // Per-vertex data for QEM simplification.
    std::vector<bool> active;
    std::vector<glm::dmat4> quadric;
    std::vector<glm::vec3> normals;
    std::vector<uint32_t> version;
    std::vector<std::vector<uint32_t>> vertFaces;
    std::vector<std::unordered_set<uint32_t>> neighbors;
    std::vector<bool> onBoundary;
    std::vector<bool> onSeam;

    std::priority_queue<Edge, std::vector<Edge>, EdgeCompare> heap;
};

inline MeshAsset simplifyMesh(const MeshAsset& input, float targetRatio) {
    return MeshSimplifier(input).simplify(targetRatio);
}
