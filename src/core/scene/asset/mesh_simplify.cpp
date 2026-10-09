#include "mesh_simplify.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <unordered_map>
#include <utility>

MeshSimplifier::MeshSimplifier(const MeshAsset& input) : vertices(input.getVertices()), indices(input.getIndices()) {
    weldVertices();
    buildAdjacency();
    constrainEdges();
    buildHeap();
}

MeshAsset MeshSimplifier::simplify(float targetRatio) {
    const size_t triCount = indices.size() / 3;
    size_t targetTris = static_cast<size_t>(std::floor(triCount * targetRatio));
    targetTris = std::max(targetTris, 8ul); // At least 8 triangle per mesh to simplify

    while (liveTris > targetTris && !heap.empty()) {
        Edge edge = heap.top();
        heap.pop();

        uint32_t v0 = edge.v0;
        uint32_t v1 = edge.v1;
        if (v0 == v1) continue;
        if (!active[v0] || !active[v1]) continue;
        if (edge.v0Version != version[v0] || edge.v1Version != version[v1]) continue;
        if (neighbors[v0].find(v1) == neighbors[v0].end()) continue;

        const EdgeFaces edgeFaces = findEdgeFaces(v0, v1);
        if (!canCollapse(edge, edgeFaces)) continue;

        collapse(edge, edgeFaces);
    }

    return buildMesh();
}

size_t MeshSimplifier::PositionHash::operator()(const glm::vec3& position) const {
    const std::hash<float> hash;
    size_t seed = hash(position.x);
    seed ^= hash(position.y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= hash(position.z) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
}

// Plane equation from triangle
glm::dmat4 MeshSimplifier::planeQuadric(const glm::dvec3& p0, const glm::dvec3& p1, const glm::dvec3& p2) {
    glm::dvec3 n = glm::normalize(glm::cross(p1 - p0, p2 - p0));
    if (!(std::isfinite(n.x) && std::isfinite(n.y) && std::isfinite(n.z))) { return glm::dmat4(0.0); }

    double d = -glm::dot(n, p0);
    glm::dvec4 plane(n, d);
    return glm::outerProduct(plane, plane);
}

glm::dmat4 MeshSimplifier::constraintQuadric(const glm::dvec3& p0, const glm::dvec3& p1, const glm::dvec3& opposite) {
    const glm::dvec3 faceNormal = glm::cross(p1 - p0, opposite - p0);
    return kConstraintWeight * planeQuadric(p0, p1, p0 + faceNormal);
}

double MeshSimplifier::quadricError(const glm::dmat4& q, const glm::dvec3& v) {
    glm::dvec4 homoV(v, 1.0);
    return glm::dot(homoV, q * homoV);
}

bool MeshSimplifier::solveOptimalPosition(const glm::dmat4& q, glm::dvec3& out) {
    glm::dmat3 a(q[0][0], q[0][1], q[0][2], q[1][0], q[1][1], q[1][2], q[2][0], q[2][1], q[2][2]);
    glm::dvec3 b(-q[0][3], -q[1][3], -q[2][3]);
    double det = glm::determinant(a);
    if (std::abs(det) < 1e-8) { return false; }

    out = glm::inverse(a) * b;
    return std::isfinite(out.x) && std::isfinite(out.y) && std::isfinite(out.z);
}

int MeshSimplifier::cornerOf(const Face& face, uint32_t v) {
    for (int k = 0; k < 3; k++) {
        if (face.v[k] == v) return k;
    }
    return -1;
}

std::vector<glm::vec3> MeshSimplifier::accumulateNormals(const std::vector<Vertex>& vertices,
                                                         const std::vector<uint32_t>& indices) {
    std::vector<glm::vec3> normals(vertices.size(), glm::vec3(0.0f));
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        const glm::vec3 p0 = vertices[indices[i + 0]].position;
        const glm::vec3 p1 = vertices[indices[i + 1]].position;
        const glm::vec3 p2 = vertices[indices[i + 2]].position;
        const glm::vec3 n = glm::cross(p1 - p0, p2 - p0);
        normals[indices[i + 0]] += n;
        normals[indices[i + 1]] += n;
        normals[indices[i + 2]] += n;
    }
    return normals;
}

void MeshSimplifier::weldVertices() {
    std::vector<uint32_t> welded(vertices.size());
    std::unordered_map<glm::vec3, uint32_t, PositionHash> weldedIds;
    weldedIds.reserve(vertices.size());
    for (size_t i = 0; i < vertices.size(); i++) {
        const auto [it, inserted] =
            weldedIds.try_emplace(vertices[i].position, static_cast<uint32_t>(positions.size()));
        if (inserted) positions.push_back(vertices[i].position);
        welded[i] = it->second;
    }

    Face face;
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        for (int k = 0; k < 3; k++) {
            face.v[k] = welded[indices[i + k]];
            face.attribute[k] = indices[i + k];
        }
        faces.push_back(face);
    }
}

void MeshSimplifier::buildAdjacency() {
    active.assign(positions.size(), true);
    quadric.assign(positions.size(), glm::dmat4(0.0));
    normals.assign(positions.size(), glm::vec3(0.0f));
    version.assign(positions.size(), 0);
    vertFaces.resize(positions.size());
    neighbors.resize(positions.size());

    for (uint32_t fi = 0; fi < faces.size(); fi++) {
        Face& face = faces[fi];
        if (isDegenerateFace(face)) {
            face.alive = false;
            continue;
        }
        liveTris++;

        const glm::vec3 p0 = positions[face.v[0]];
        const glm::vec3 p1 = positions[face.v[1]];
        const glm::vec3 p2 = positions[face.v[2]];
        glm::dmat4 q = planeQuadric(p0, p1, p2);
        // Accumulate plane quadrics into each incident vertex.
        quadric[face.v[0]] += q;
        quadric[face.v[1]] += q;
        quadric[face.v[2]] += q;

        const glm::vec3 n = glm::cross(p1 - p0, p2 - p0);
        normals[face.v[0]] += n;
        normals[face.v[1]] += n;
        normals[face.v[2]] += n;
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

    for (glm::vec3& normal : normals) {
        const float length = glm::length(normal);
        if (length > 0.0f) normal /= length;
    }
}

void MeshSimplifier::constrainEdges() {
    onBoundary.assign(positions.size(), false);
    onSeam.assign(positions.size(), false);

    for (uint32_t v0 = 0; v0 < positions.size(); v0++) {
        for (uint32_t v1 : neighbors[v0]) {
            if (v1 <= v0) continue;

            const EdgeFaces edgeFaces = findEdgeFaces(v0, v1);
            const bool seam = isSeam(edgeFaces, v0, v1);
            if (edgeFaces.count == 2 && !seam) continue;

            std::vector<bool>& constrained = seam ? onSeam : onBoundary;
            constrained[v0] = true;
            constrained[v1] = true;

            for (uint32_t i = 0; i < std::min(edgeFaces.count, 2u); i++) {
                const Face& face = faces[edgeFaces.faces[i]];
                const uint32_t opposite = face.v[3 - cornerOf(face, v0) - cornerOf(face, v1)];
                const glm::dmat4 q = constraintQuadric(positions[v0], positions[v1], positions[opposite]);
                quadric[v0] += q;
                quadric[v1] += q;
            }
        }
    }
}

void MeshSimplifier::buildHeap() {
    for (uint32_t v0 = 0; v0 < positions.size(); v0++) {
        for (uint32_t v1 : neighbors[v0]) {
            if (v1 <= v0) continue;
            heap.push(computeEdge(v0, v1));
        }
    }
}

bool MeshSimplifier::isDegenerateFace(const Face& face) const {
    uint32_t i0 = face.v[0];
    uint32_t i1 = face.v[1];
    uint32_t i2 = face.v[2];
    if (i0 == i1 || i1 == i2 || i0 == i2) { return true; }

    const glm::vec3 p0 = positions[i0];
    const glm::vec3 p1 = positions[i1];
    const glm::vec3 p2 = positions[i2];
    float areaSq = glm::length(glm::cross(p1 - p0, p2 - p0));
    return areaSq < 1e-12f;
}

bool MeshSimplifier::colorsDiffer(const Face& a, const Face& b, uint32_t v) const {
    return vertices[a.attribute[cornerOf(a, v)]].color != vertices[b.attribute[cornerOf(b, v)]].color;
}

MeshSimplifier::EdgeFaces MeshSimplifier::findEdgeFaces(uint32_t v0, uint32_t v1) const {
    EdgeFaces edgeFaces;
    for (uint32_t fi : vertFaces[v1]) {
        if (!faces[fi].alive || cornerOf(faces[fi], v0) < 0) continue;
        if (edgeFaces.count < 2) edgeFaces.faces[edgeFaces.count] = fi;
        edgeFaces.count++;
    }
    return edgeFaces;
}

bool MeshSimplifier::isSeam(const EdgeFaces& edgeFaces, uint32_t v0, uint32_t v1) const {
    if (edgeFaces.count != 2) return false;
    const Face& a = faces[edgeFaces.faces[0]];
    const Face& b = faces[edgeFaces.faces[1]];
    return colorsDiffer(a, b, v0) || colorsDiffer(a, b, v1);
}

float MeshSimplifier::topologyCost(uint32_t v0, uint32_t v1) const {
    const glm::vec3 edgeVector = positions[v1] - positions[v0];
    return std::abs(glm::dot(normals[v0], normals[v1])) /
           std::min(-glm::dot(edgeVector, edgeVector), -std::numeric_limits<float>::epsilon());
}

MeshSimplifier::Edge MeshSimplifier::computeEdge(uint32_t v0, uint32_t v1) const {
    if (v1 < v0) std::swap(v0, v1);
    glm::dmat4 q = quadric[v0] + quadric[v1];
    glm::dvec3 pos;
    if (!solveOptimalPosition(q, pos)) { pos = 0.5 * (glm::dvec3(positions[v0]) + glm::dvec3(positions[v1])); }
    float cost = static_cast<float>(std::abs(quadricError(q, pos)));
    if (cost < kTopologyFallbackEpsilon) cost = topologyCost(v0, v1) - cost;
    return Edge{v0, v1, glm::vec3(pos), cost, version[v0], version[v1]};
}

bool MeshSimplifier::flipsFaces(uint32_t moved, uint32_t other, const glm::vec3& position) const {
    for (uint32_t fi : vertFaces[moved]) {
        const Face& face = faces[fi];
        if (!face.alive || cornerOf(face, other) >= 0) continue;

        glm::vec3 p[3] = {positions[face.v[0]], positions[face.v[1]], positions[face.v[2]]};
        const glm::vec3 before = glm::cross(p[1] - p[0], p[2] - p[0]);
        p[cornerOf(face, moved)] = position;
        const glm::vec3 after = glm::cross(p[1] - p[0], p[2] - p[0]);
        if (glm::dot(before, after) <= (glm::dot(before, before) + glm::dot(after, after)) * kFlipTolerance)
            return true;
    }
    return false;
}

bool MeshSimplifier::canCollapse(const Edge& edge, const EdgeFaces& edgeFaces) const {
    const uint32_t v0 = edge.v0;
    const uint32_t v1 = edge.v1;
    if (edgeFaces.count == 0 || edgeFaces.count > 2) return false;

    uint32_t sharedNeighbors = 0;
    for (uint32_t n : neighbors[v1]) {
        if (neighbors[v0].contains(n)) sharedNeighbors++;
    }
    if (sharedNeighbors != edgeFaces.count) return false;

    if (onBoundary[v0] && onBoundary[v1] && edgeFaces.count != 1) return false;
    if (onSeam[v0] && onSeam[v1] && !isSeam(edgeFaces, v0, v1)) return false;

    return !flipsFaces(v0, v1, edge.position) && !flipsFaces(v1, v0, edge.position);
}

void MeshSimplifier::collapse(const Edge& edge, const EdgeFaces& edgeFaces) {
    const uint32_t v0 = edge.v0;
    const uint32_t v1 = edge.v1;

    std::pair<uint32_t, uint32_t> attributeMerges[2];
    for (uint32_t i = 0; i < edgeFaces.count; i++) {
        const Face& face = faces[edgeFaces.faces[i]];
        attributeMerges[i] = {face.attribute[cornerOf(face, v1)], face.attribute[cornerOf(face, v0)]};
    }

    const glm::vec3 edgeVector = positions[v1] - positions[v0];
    const float edgeLengthSquared = glm::dot(edgeVector, edgeVector);
    const float factor =
        edgeLengthSquared > 0.0f ? glm::dot(edge.position - positions[v0], edgeVector) / edgeLengthSquared : 0.5f;
    const glm::vec3 normal = glm::mix(normals[v0], normals[v1], factor);
    const float normalLength = glm::length(normal);
    normals[v0] = normalLength > 0.0f ? normal / normalLength : glm::vec3(0.0f);

    // Collapse v1 into v0 (keep v0 as the survivor).
    positions[v0] = edge.position;
    quadric[v0] += quadric[v1];
    active[v1] = false;
    version[v0]++;
    onBoundary[v0] = onBoundary[v0] || onBoundary[v1];
    onSeam[v0] = onSeam[v0] || onSeam[v1];

    // Update faces
    for (uint32_t fi : vertFaces[v1]) {
        Face& face = faces[fi];
        if (!face.alive) continue;

        if (cornerOf(face, v0) >= 0) {
            face.alive = false;
            if (liveTris > 0) liveTris--;
            continue;
        }

        const int corner = cornerOf(face, v1);
        face.v[corner] = v0;
        for (uint32_t i = 0; i < edgeFaces.count; i++) {
            if (face.attribute[corner] != attributeMerges[i].first) continue;
            face.attribute[corner] = attributeMerges[i].second;
            break;
        }
        vertFaces[v0].push_back(fi);
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
    for (uint32_t n : neighbors[v0]) { heap.push(computeEdge(v0, n)); }
    for (uint32_t n : neighbors[v0]) {
        for (uint32_t m : neighbors[n]) {
            if (m > n && neighbors[v0].contains(m)) heap.push(computeEdge(n, m));
        }
    }
}

MeshAsset MeshSimplifier::buildMesh() const {
    const std::vector<glm::vec3> windingNormals = accumulateNormals(vertices, indices);

    std::vector<Vertex> newVertices;
    std::vector<uint32_t> newIndices;
    std::vector<bool> inverted;
    std::unordered_map<uint64_t, uint32_t> newVertexIds;
    for (const Face& face : faces) {
        if (!face.alive) continue;
        for (int k = 0; k < 3; k++) {
            const uint32_t attribute = face.attribute[k];
            const uint64_t key = static_cast<uint64_t>(face.v[k]) | (static_cast<uint64_t>(attribute) << 32);
            const auto [it, inserted] = newVertexIds.try_emplace(key, static_cast<uint32_t>(newVertices.size()));
            if (inserted) {
                Vertex vertex = vertices[attribute];
                vertex.position = positions[face.v[k]];
                newVertices.push_back(vertex);
                inverted.push_back(glm::dot(windingNormals[attribute], vertices[attribute].normal) < 0.0f);
            }
            newIndices.push_back(it->second);
        }
    }

    const std::vector<glm::vec3> normals = accumulateNormals(newVertices, newIndices);
    for (size_t i = 0; i < newVertices.size(); i++) {
        const float length = glm::length(normals[i]);
        if (!(length > 0.0f)) continue;
        newVertices[i].normal = inverted[i] ? -normals[i] / length : normals[i] / length;
    }

    return MeshAsset(std::move(newVertices), std::move(newIndices));
}
