#include "mesh_simplify.hpp"

#include <iostream>
#include <unordered_set>
#include <vector>
#include <queue>
#include <cmath>

struct Face {
    uint32_t v[3];
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
    bool operator()(const Edge& a, const Edge& b) const {  return a.cost > b.cost; }
};


// Plane equation from triangle
glm::mat4 planeQuadric(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2) {
    glm::vec3 n = glm::normalize(glm::cross(p1 - p0, p2 - p0));
    if (!(std::isfinite(n.x) && std::isfinite(n.y) && std::isfinite(n.z))) { return glm::mat4(0.0f); }

    float d = -glm::dot(n, p0);
    glm::vec4 plane(n, d);
    return glm::outerProduct(plane, plane);
}

float quadricError(const glm::mat4& q, const glm::vec3& v) {
    glm::vec4 homoV(v, 1.0f);
    return glm::dot(homoV, q * homoV);
}

bool solveOptimalPosition(const glm::mat4& q, glm::vec3& out) {
    glm::mat3 a(
        q[0][0], q[0][1], q[0][2],
        q[1][0], q[1][1], q[1][2],
        q[2][0], q[2][1], q[2][2]
    );
    glm::vec3 b(-q[0][3], -q[1][3], -q[2][3]);
    float det = glm::determinant(a);
    if (std::abs(det) < 1e-8f) { return false; }

    out = glm::inverse(a) * b;
    return std::isfinite(out.x) && std::isfinite(out.y) && std::isfinite(out.z);
}

bool isDegenerateFace(const Face& face, const std::vector<Vertex>& vertices) {
    uint32_t i0 = face.v[0];
    uint32_t i1 = face.v[1];
    uint32_t i2 = face.v[2];
    if (i0 == i1 || i1 == i2 || i0 == i2) { return true; }

    const glm::vec3 p0 = vertices[i0].position;
    const glm::vec3 p1 = vertices[i1].position;
    const glm::vec3 p2 = vertices[i2].position;
    float areaSq = glm::length(glm::cross(p1 - p0, p2 - p0));
    return areaSq < 1e-12f;
}

void remapIndices(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
    std::vector<uint32_t> remap(vertices.size(), UINT32_MAX);
    std::vector<Vertex> newVertices;

    for (uint32_t& idx : indices) {
        if (remap[idx] == UINT32_MAX) {
            remap[idx] = static_cast<uint32_t>(newVertices.size());
            newVertices.push_back(vertices[idx]);
        }
        idx = remap[idx];
    }

    vertices.swap(newVertices);
}

Edge computeEdge(
    uint32_t v0,
    uint32_t v1,
    const std::vector<glm::mat4>& quadric,
    const std::vector<Vertex>& vertices,
    const std::vector<uint32_t>& version
) {
    if (v1 < v0) std::swap(v0, v1);
    glm::mat4 q = quadric[v0] + quadric[v1];
    glm::vec3 pos;
    if (!solveOptimalPosition(q, pos)) {
        pos = 0.5f * (vertices[v0].position + vertices[v1].position);
    }
    float cost = quadricError(q, pos);
    return Edge{ v0, v1, pos, cost, version[v0], version[v1] };
}

MeshAsset simplifyMesh(const MeshAsset& input, float targetRatio) {
    std::vector<Vertex> vertices = input.getVertices();
    std::vector<uint32_t> indices = input.getIndices();

    const size_t triCount = indices.size() / 3;
    size_t targetTris = static_cast<size_t>(std::floor(triCount * targetRatio));
    targetTris = std::max(targetTris, 1ul); // Need to have at least on triangle to work

    std::vector<Face> faces;
    Face face;
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        face.v[0] = indices[i + 0];
        face.v[1] = indices[i + 1];
        face.v[2] = indices[i + 2];
        faces.push_back(face);
    }

    // Per-vertex data for QEM simplification.
    std::vector<bool> active(vertices.size(), true);
    std::vector<glm::mat4> quadric(vertices.size(), glm::mat4(0.0f));
    std::vector<uint32_t> version(vertices.size(), 0);
    std::vector<std::vector<uint32_t> > vertFaces(vertices.size());
    std::vector<std::unordered_set<uint32_t> > neighbors(vertices.size());

    for (uint32_t fi = 0; fi < faces.size(); fi++) {
        Face& face = faces[fi];
        if (isDegenerateFace(face, vertices)) {
            face.alive = false;
            continue;
        }
        const glm::vec3 p0 = vertices[face.v[0]].position;
        const glm::vec3 p1 = vertices[face.v[1]].position;
        const glm::vec3 p2 = vertices[face.v[2]].position;
        glm::mat4 q = planeQuadric(p0, p1, p2);
        // Accumulate plane quadrics into each incident vertex.
        quadric[face.v[0]] += q;
        quadric[face.v[1]] += q;
        quadric[face.v[2]] += q;
        vertFaces[face.v[0]].push_back(fi);
        vertFaces[face.v[1]].push_back(fi);
        vertFaces[face.v[2]].push_back(fi);

        neighbors[face.v[0]].insert(face.v[1]);
        neighbors[face.v[0]].insert(face.v[2]);
        neighbors[face.v[1]].insert(face.v[0]);
        neighbors[face.v[1]].insert(face.v[2]);
        neighbors[face.v[2]].insert(face.v[0]);
        neighbors[face.v[2]].insert(face.v[1]);
    }

    size_t liveTris = 0;
    for (const Face& f : faces) {
        if (f.alive)
            liveTris++;
    }

    std::priority_queue<Edge, std::vector<Edge>, EdgeCompare> heap;
    for (uint32_t v0 = 0; v0 < vertices.size(); v0++) {
        for (uint32_t v1 : neighbors[v0]) {
            if (v1 <= v0) continue;
            heap.push(computeEdge(v0, v1, quadric, vertices, version));
        }
    }

    while (liveTris > targetTris && !heap.empty()) {
        Edge edge = heap.top();
        heap.pop();

        uint32_t v0 = edge.v0;
        uint32_t v1 = edge.v1;
        if (v0 == v1) continue;
        if (!active[v0] || !active[v1]) continue;
        if (edge.v0Version != version[v0] || edge.v1Version != version[v1]) continue;
        if (neighbors[v0].find(v1) == neighbors[v0].end()) continue;

        // Collapse v1 into v0 (keep v0 as the survivor).
        vertices[v0].position = edge.position;
        quadric[v0] += quadric[v1];
        active[v1] = false;
        version[v0]++;

        // Update faces
        for (uint32_t fi : vertFaces[v1]) {
            Face& face = faces[fi];
            if (!face.alive) continue;
            for (int k = 0; k < 3; k++) {
                if (face.v[k] == v1) face.v[k] = v0;
            }
            if (isDegenerateFace(face, vertices)) {
                face.alive = false;
                if (liveTris > 0) liveTris--;
            } else {
                vertFaces[v0].push_back(fi);
            }
        }

        // Update neighbors
        for (uint32_t n : neighbors[v1]) {
            neighbors[n].erase(v1);
            if (n != v0) {
                neighbors[n].insert(v0);
                neighbors[v0].insert(n);
            }
        }
        neighbors[v1].clear();
        neighbors[v0].erase(v0);

        // Recompute edge costs for v0 neighborhood.
        for (uint32_t n : neighbors[v0]) {
            heap.push(computeEdge(v0, n, quadric, vertices, version));
        }
    }

    indices.clear();
    for (const Face& face : faces) {
        if (!face.alive) continue;
        indices.push_back(face.v[0]);
        indices.push_back(face.v[1]);
        indices.push_back(face.v[2]);
    }

    remapIndices(vertices, indices);

    MeshAsset mesh = MeshAsset(
        input.getName(),
        std::move(vertices),
        std::move(indices)
    );
    mesh.setPath(input.getPath());
    return mesh;
}
