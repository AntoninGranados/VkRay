#include "mesh.hpp"

#include <algorithm>
#include <format>
#include <unordered_map>
#include <limits>
#include <numeric>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tinyobjloader/tiny_obj_loader.h"

#include "utils/log.hpp"

MeshAsset makeDefaultMeshAsset() {
    return MeshAsset(
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

MeshAsset::MeshAsset(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices):
vertices(vertices), indices(indices) {
    buildBvh();
}

std::optional<MeshAsset> MeshAsset::load(const std::string& path) {
    MeshAsset asset;

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;
    std::string baseDir = std::filesystem::path(path).parent_path().string() + "/";
    bool loaded = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), baseDir.c_str(), true);
    if (!warn.empty()) Log::warn("Mesh", std::format("Issue when loading .obj: {}", warn));
    if (!err.empty())  Log::error("Mesh", std::format("Failed to load .obj: {}", err));
    if (!loaded) {
        Log::error("Mesh", std::format("Failed to load .obj: {}", path));
        return std::nullopt;
    }

    std::vector<Vertex>& vertices = asset.vertices;
    std::vector<uint32_t>& indices = asset.indices;

    vertices.reserve(attrib.vertices.size() / 3);
    for (size_t i = 0; i + 2 < attrib.vertices.size(); i += 3) {
        vertices.push_back(Vertex{
            .position = glm::vec3(attrib.vertices[i + 0], attrib.vertices[i + 1], attrib.vertices[i + 2]),
            .normal = glm::vec3(0.0f),
        });
    }

    std::vector<int> vertexMaterial(vertices.size(), -1);
    std::unordered_map<uint64_t, unsigned int> splitVertices;

    for (const tinyobj::shape_t& shape : shapes) {
        size_t indexOffset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            int fv = shape.mesh.num_face_vertices[f];
            if (fv != 3) {
                indexOffset += fv;
                continue;
            }

            int matId = shape.mesh.material_ids.empty() ? -1 : shape.mesh.material_ids[f];
            const bool hasMat = matId >= 0 && matId < (int)materials.size();

            glm::vec3 faceColor(0.0f);
            if (hasMat) {
                const auto& kd = materials[matId].diffuse;
                faceColor = glm::vec3(kd[0], kd[1], kd[2]);
                asset.vertexColorLoaded = true;
            }

            for (int v = 0; v < fv; v++) {
                const tinyobj::index_t idx = shape.mesh.indices[indexOffset + v];
                if (idx.vertex_index < 0) continue;
                const int origIdx = idx.vertex_index;

                if (!hasMat) {
                    indices.push_back(static_cast<unsigned int>(origIdx));
                    continue;
                }

                if (vertexMaterial[origIdx] == -1) {
                    vertices[origIdx].color = faceColor;
                    vertexMaterial[origIdx] = matId;
                    indices.push_back(static_cast<unsigned int>(origIdx));
                } else if (vertexMaterial[origIdx] == matId) {
                    indices.push_back(static_cast<unsigned int>(origIdx));
                } else {
                    const uint64_t key = static_cast<uint64_t>(origIdx) | (static_cast<uint64_t>(matId) << 32);
                    auto it = splitVertices.find(key);
                    if (it != splitVertices.end()) {
                        indices.push_back(it->second);
                    } else {
                        const unsigned int newIdx = static_cast<unsigned int>(vertices.size());
                        vertices.push_back(Vertex{ .position = vertices[origIdx].position, .color = faceColor });
                        splitVertices[key] = newIdx;
                        indices.push_back(newIdx);
                    }
                }
            }
            indexOffset += fv;
        }
    }

    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        uint32_t i0 = indices[i], i1 = indices[i+1], i2 = indices[i+2];
        glm::vec3 e1 = vertices[i1].position - vertices[i0].position;
        glm::vec3 e2 = vertices[i2].position - vertices[i0].position;
        glm::vec3 n  = glm::cross(e1, e2);
        vertices[i0].normal += n;
        vertices[i1].normal += n;
        vertices[i2].normal += n;
    }
    for (Vertex& v : vertices) v.normal = glm::normalize(v.normal);

    asset.buildBvh();

    return asset;
}

void MeshAsset::buildBvh() {
    bvhNodes.clear();
    const size_t triCount = indices.size() / 3;
    if (triCount == 0) return;

    std::vector<TriBounds> triBounds(triCount);
    for (size_t i = 0; i < triCount; i++) {
        const glm::vec3 v0 = vertices[indices[i * 3 + 0]].position;
        const glm::vec3 v1 = vertices[indices[i * 3 + 1]].position;
        const glm::vec3 v2 = vertices[indices[i * 3 + 2]].position;
        glm::vec3 mn = glm::min(v0, glm::min(v1, v2));
        glm::vec3 mx = glm::max(v0, glm::max(v1, v2));
        triBounds[i] = { mn, mx, (v0 + v1 + v2) / 3.0f };
    }

    std::vector<uint32_t> triIndices(triCount);
    std::iota(triIndices.begin(), triIndices.end(), 0u);

    buildBvhNode(triBounds, triIndices, 0, triCount, aabbMin, aabbMax);

    std::vector<unsigned int> reordered(indices.size());
    for (size_t newTri = 0; newTri < triCount; newTri++) {
        const size_t oldTri = triIndices[newTri];
        reordered[newTri * 3 + 0] = indices[oldTri * 3 + 0];
        reordered[newTri * 3 + 1] = indices[oldTri * 3 + 1];
        reordered[newTri * 3 + 2] = indices[oldTri * 3 + 2];
    }
    indices.swap(reordered);
}

float MeshAsset::computeArea(const glm::mat4& transform) const {
    if (indices.empty()) return 0.0f;
    const glm::mat3 linear(transform);
    double area = 0.0;
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        const uint32_t i0 = indices[i + 0];
        const uint32_t i1 = indices[i + 1];
        const uint32_t i2 = indices[i + 2];
        if (i0 >= vertices.size() || i1 >= vertices.size() || i2 >= vertices.size())
            continue;
        const glm::vec3 v0 = vertices[i0].position;
        const glm::vec3 v1 = vertices[i1].position;
        const glm::vec3 v2 = vertices[i2].position;
        const glm::vec3 e1 = linear * (v1 - v0);
        const glm::vec3 e2 = linear * (v2 - v0);
        area += 0.5 * glm::length(glm::cross(e1, e2));
    }
    return static_cast<float>(area);
}

namespace {
    struct TriangleBin {
        glm::vec3 aabbMin = glm::vec3( std::numeric_limits<float>::infinity());
        glm::vec3 aabbMax = glm::vec3(-std::numeric_limits<float>::infinity());
        int count = 0;
    };
    float SurfaceAreaHeuristic(glm::vec3 mn, glm::vec3 mx) {
        glm::vec3 d = mx - mn;
        return 2.0f * (d.x*d.y + d.y*d.z + d.z*d.x);
    }
}

size_t MeshAsset::buildBvhNode(std::vector<TriBounds>& triBounds, std::vector<uint32_t>& triIndices, uint32_t start, uint32_t count, glm::vec3& outAabbMin, glm::vec3& outAabbMax) {
    size_t nodeIndex = bvhNodes.size();
    bvhNodes.push_back({});

    glm::vec3 nodeMin(std::numeric_limits<float>::infinity());
    glm::vec3 nodeMax(-std::numeric_limits<float>::infinity());
    glm::vec3 centroidMin(std::numeric_limits<float>::infinity());
    glm::vec3 centroidMax(-std::numeric_limits<float>::infinity());
    for (size_t i = 0; i < count; i++) {
        const TriBounds& tb = triBounds[triIndices[start + i]];
        nodeMin = glm::min(nodeMin, tb.min);
        nodeMax = glm::max(nodeMax, tb.max);
        centroidMin = glm::min(centroidMin, tb.centroid);
        centroidMax = glm::max(centroidMax, tb.centroid);
    }
    outAabbMin = nodeMin;
    outAabbMax = nodeMax;

    auto makeLeaf = [&]() {
        bvhNodes[nodeIndex].firstTriangle = start;
        bvhNodes[nodeIndex].triangleCount = count;
    };

    glm::vec3 extent = centroidMax - centroidMin;
    if (count <= kLeafSize || (extent.x < 1e-3f && extent.y < 1e-3f && extent.z < 1e-3f)) {
        makeLeaf();
        return nodeIndex;
    }

    const float triCost  = 1.0f;
    const float aabbCost = 0.125f;

    int   bestAxis = -1, bestBin = -1;
    float bestCost = std::numeric_limits<float>::infinity();
    float bestScale = 0.0f;

    for (int axis = 0; axis < 3; axis++) {
        if (extent[axis] < 1e-3f) continue;
        float scale = kSahK / extent[axis];

        std::vector<TriangleBin> bins(kSahK);
        for (size_t i = 0; i < count; i++) {
            const TriBounds& tb = triBounds[triIndices[start + i]];
            int b = std::clamp(int((tb.centroid[axis] - centroidMin[axis]) * scale), 0, kSahK - 1);
            bins[b].aabbMax = glm::max(bins[b].aabbMax, tb.max);
            bins[b].aabbMin = glm::min(bins[b].aabbMin, tb.min);
            bins[b].count++;
        }

        std::vector<TriangleBin> lSweep(kSahK - 1), rSweep(kSahK - 1);
        lSweep[0] = bins[0];
        rSweep[0] = bins[kSahK - 1];
        for (int i = 1; i < kSahK - 1; i++) {
            lSweep[i].aabbMax = glm::max(lSweep[i-1].aabbMax, bins[i].aabbMax);
            lSweep[i].aabbMin = glm::min(lSweep[i-1].aabbMin, bins[i].aabbMin);
            lSweep[i].count   = lSweep[i-1].count + bins[i].count;
            rSweep[i].aabbMax = glm::max(rSweep[i-1].aabbMax, bins[kSahK-1-i].aabbMax);
            rSweep[i].aabbMin = glm::min(rSweep[i-1].aabbMin, bins[kSahK-1-i].aabbMin);
            rSweep[i].count   = rSweep[i-1].count + bins[kSahK-1-i].count;
        }

        for (int i = 0; i < kSahK - 1; i++) {
            float cost = SurfaceAreaHeuristic(lSweep[i].aabbMin, lSweep[i].aabbMax) * lSweep[i].count
                       + SurfaceAreaHeuristic(rSweep[kSahK-2-i].aabbMin, rSweep[kSahK-2-i].aabbMax) * rSweep[kSahK-2-i].count;
            if (cost < bestCost) {
                bestCost  = cost;
                bestBin   = i;
                bestAxis  = axis;
                bestScale = scale;
            }
        }
    }

    float splitCost = aabbCost + bestCost / SurfaceAreaHeuristic(nodeMin, nodeMax);
    float leafCost  = count * triCost;

    if (bestAxis == -1 || splitCost >= leafCost) {
        makeLeaf();
        return nodeIndex;
    }

    auto midIt = std::partition(
        triIndices.begin() + start,
        triIndices.begin() + start + count,
        [&](uint32_t tri) {
            int b = std::clamp(int((triBounds[tri].centroid[bestAxis] - centroidMin[bestAxis]) * bestScale), 0, kSahK - 1);
            return b <= bestBin;
        }
    );

    uint32_t leftCount  = std::distance(triIndices.begin() + start, midIt);
    uint32_t rightCount = count - leftCount;

    if (leftCount == 0 || rightCount == 0) {
        makeLeaf();
        return nodeIndex;
    }

    glm::vec3 leftMin, leftMax, rightMin, rightMax;
    uint32_t left  = buildBvhNode(triBounds, triIndices, start,             leftCount,  leftMin,  leftMax);
    uint32_t right = buildBvhNode(triBounds, triIndices, start + leftCount, rightCount, rightMin, rightMax);

    GpuBvhNode& node = bvhNodes[nodeIndex];
    node.children[0] = { leftMin.x,  leftMin.y,  leftMin.z,  leftMax.x,  leftMax.y,  leftMax.z,  left  };
    node.children[1] = { rightMin.x, rightMin.y, rightMin.z, rightMax.x, rightMax.y, rightMax.z, right };
    node.firstTriangle = 0u;
    node.triangleCount = 0u;
    return nodeIndex;
}

