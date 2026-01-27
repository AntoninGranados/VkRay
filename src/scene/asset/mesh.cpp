#include "mesh.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "../../notification_system.hpp"

bool MeshAsset::loadFromObj(const AppContext& ctx, const std::string& _path) {
    path = _path;

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;
    bool loaded = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), nullptr, true);
    if (!warn.empty()) {
        ctx.notifications->pushMessage(NotificationType::Warning, "Failed to load obj: " + warn);
    }
    if (!err.empty()) {
        ctx.notifications->pushMessage(NotificationType::Error, "Failed to load obj: " + err);
    }
    if (!loaded) {
        ctx.notifications->pushMessage(NotificationType::Error, "Failed to load obj [" + path + "]");
        return false;
    }

    vertices.clear();
    vertices.reserve(attrib.vertices.size() / 3);
    for (size_t i = 0; i + 2 < attrib.vertices.size(); i += 3) {
        vertices.push_back(Vertex{
            .position = glm::vec3(attrib.vertices[i + 0], attrib.vertices[i + 1], attrib.vertices[i + 2]),
        });
    }

    indices.clear();
    for (const tinyobj::shape_t &shape : shapes) {
        size_t indexOffset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            int fv = shape.mesh.num_face_vertices[f];
            if (fv != 3) {
                indexOffset += fv;
                continue;
            }

            for (int v = 0; v < fv; v++) {
                const tinyobj::index_t idx = shape.mesh.indices[indexOffset + v];
                if (idx.vertex_index < 0) continue;
                indices.push_back(static_cast<unsigned int>(idx.vertex_index));
            }
            indexOffset += fv;
        }
    }

    buildBvh();

    return true;
}

size_t MeshAsset::buildBvhNode(std::vector<TriBounds> &triBounds, std::vector<uint32_t> &triIndices, uint32_t start, uint32_t count) {
    uint32_t nodeIndex = bvhNodes.size();
    bvhNodes.push_back({});

    glm::vec3 nodeMin(std::numeric_limits<float>::infinity());
    glm::vec3 nodeMax(-std::numeric_limits<float>::infinity());
    glm::vec3 centroidMin(std::numeric_limits<float>::infinity());
    glm::vec3 centroidMax(-std::numeric_limits<float>::infinity());
    for (size_t i = 0; i < count; i++) {
        const TriBounds &tb = triBounds[triIndices[start + i]];
        nodeMin = glm::min(nodeMin, tb.min);
        nodeMax = glm::max(nodeMax, tb.max);
        centroidMin = glm::min(centroidMin, tb.centroid);
        centroidMax = glm::max(centroidMax, tb.centroid);
    }

    if (count <= LEAF_SIZE) {
        bvhNodes[nodeIndex] = {
            .aabbMin = nodeMin,
            .aabbMax = nodeMax,
            .data0 = start,
            .data1 = count,
            .isLeaf = 1,
        };
        return nodeIndex;
    }

    glm::vec3 extent = centroidMax - centroidMin;
    int axis = 0;
    if (extent.y > extent.x && extent.y >= extent.z) axis = 1;
    else if (extent.z > extent.x) axis = 2;

    uint32_t mid = start + count / 2;
    std::nth_element(
        triIndices.begin() + start,
        triIndices.begin() + mid,
        triIndices.begin() + start + count,
        [&](uint32_t a, uint32_t b) {
            return triBounds[a].centroid[axis] < triBounds[b].centroid[axis];
        }
    );

    uint32_t leftCount = mid - start;
    uint32_t rightCount = count - leftCount;
    if (leftCount == 0 || rightCount == 0) {
        mid = start + count / 2;
        leftCount = mid - start;
        rightCount = count - leftCount;
    }

    uint32_t left = buildBvhNode(triBounds, triIndices ,start, leftCount);
    uint32_t right = buildBvhNode(triBounds, triIndices ,mid, rightCount);

    bvhNodes[nodeIndex] = {
        .aabbMin = nodeMin,
        .aabbMax = nodeMax,
        .data0 = left,
        .data1 = right,
        .isLeaf = 0,
    };
    return nodeIndex;
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
        triBounds[i] = { mn, mx, (mn + mx) * 0.5f };
    }

    std::vector<uint32_t> triIndices(triCount);
    std::iota(triIndices.begin(), triIndices.end(), 0u);

    buildBvhNode(triBounds, triIndices, 0, triCount);

    std::vector<unsigned int> reordered(indices.size());
    for (size_t newTri = 0; newTri < triCount; newTri++) {
        const size_t oldTri = triIndices[newTri];
        reordered[newTri * 3 + 0] = indices[oldTri * 3 + 0];
        reordered[newTri * 3 + 1] = indices[oldTri * 3 + 1];
        reordered[newTri * 3 + 2] = indices[oldTri * 3 + 2];
    }
    indices.swap(reordered);
}
