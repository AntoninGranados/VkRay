#include "mesh.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tinyobjloader/tiny_obj_loader.h"

#include "mesh_simplify.hpp"
#include "app/notification_handler.hpp"

// Public
MeshAsset::MeshAsset(const std::string& name, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices):
name(name), vertices(vertices), indices(indices) {
    savedVertices = this->vertices;
    savedIndices = this->indices;
    baseVertices = this->vertices;
    baseIndices = this->indices;
    hasSaved = true;
    buildBvh();
}

bool MeshAsset::loadFromObj(const AppContext& ctx, const std::string& _path) {
    path = _path;

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;
    std::string baseDir = std::filesystem::path(path).parent_path().string() + "/";
    bool loaded = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), baseDir.c_str(), true);
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
            .normal = glm::vec3(0.0f),
        });
    }

    indices.clear();
    vertexColorLoaded = false;

    for (const tinyobj::shape_t& shape : shapes) {
        size_t indexOffset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            int fv = shape.mesh.num_face_vertices[f];
            if (fv != 3) {
                indexOffset += fv;
                continue;
            }

            int matId = shape.mesh.material_ids.empty() ? -1 : shape.mesh.material_ids[f];
            if (matId >= 0 && matId < (int)materials.size()) {
                const auto& kd = materials[matId].diffuse;
                glm::vec3 faceColor(kd[0], kd[1], kd[2]);
                for (int v = 0; v < fv; v++) {
                    const tinyobj::index_t idx = shape.mesh.indices[indexOffset + v];
                    if (idx.vertex_index < 0) continue;
                    indices.push_back(static_cast<unsigned int>(idx.vertex_index));
                    vertices[idx.vertex_index].color = faceColor;
                }
                vertexColorLoaded = true;
            } else {
                for (int v = 0; v < fv; v++) {
                    const tinyobj::index_t idx = shape.mesh.indices[indexOffset + v];
                    if (idx.vertex_index < 0) continue;
                    indices.push_back(static_cast<unsigned int>(idx.vertex_index));
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

    buildBvh();
    savedVertices = vertices;
    savedIndices = indices;
    baseVertices = vertices;
    baseIndices = indices;
    hasSaved = true;
    simplifyRatio = 1.0f;

    return true;
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

std::string MeshAsset::nameFromPath(const std::string& path) {
    if (path.empty()) return "Mesh";
    size_t end = path.find_last_of('.');
    size_t slash = path.find_last_of("/\\");
    if (end == std::string::npos || end < slash) end = path.size();
    size_t start = (slash == std::string::npos) ? 0 : slash + 1;
    if (start >= end) return "Mesh";

    std::string name = path.substr(start, end - start);
    name[0] = std::toupper(name[0]);
    return name;
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

bool MeshAsset::setSimplifyRatio(float ratio) {
    ratio = glm::clamp(ratio, 0.01f, 1.0f);
    if (std::abs(ratio - simplifyRatio) < 1e-6f) return false;

    if (ratio >= 1.0f - 1e-6f) {
        simplifyRatio = 1.0f;
        if (baseVertices.empty() || baseIndices.empty()) return false;
        vertices = baseVertices;
        indices = baseIndices;
        buildBvh();
        return true;
    }

    if (baseVertices.empty() || baseIndices.empty()) {
        baseVertices = vertices;
        baseIndices = indices;
    }

    MeshAsset base(name, baseVertices, baseIndices);
    base.setPath(path);
    MeshAsset simplified = simplifyMesh(base, ratio);
    vertices = simplified.getVertices();
    indices = simplified.getIndices();
    buildBvh();
    simplifyRatio = ratio;
    return true;
}

bool MeshAsset::applySimplification() {
    if (std::abs(simplifyRatio - 1.0f) < 1e-6f) return false;
    baseVertices = vertices;
    baseIndices = indices;
    simplifyRatio = 1.0f;
    return true;
}

bool MeshAsset::revertSimplification() {
    if (!hasSaved) return false;
    baseVertices = savedVertices;
    baseIndices = savedIndices;
    vertices = baseVertices;
    indices = baseIndices;
    buildBvh();
    simplifyRatio = 1.0f;
    return true;
}

// Private helpers
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
    if (count <= LEAF_SIZE || (extent.x < 1e-3f && extent.y < 1e-3f && extent.z < 1e-3f)) {
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
        float scale = SAH_K / extent[axis];

        std::vector<TriangleBin> bins(SAH_K);
        for (size_t i = 0; i < count; i++) {
            const TriBounds& tb = triBounds[triIndices[start + i]];
            int b = std::clamp(int((tb.centroid[axis] - centroidMin[axis]) * scale), 0, SAH_K - 1);
            bins[b].aabbMax = glm::max(bins[b].aabbMax, tb.max);
            bins[b].aabbMin = glm::min(bins[b].aabbMin, tb.min);
            bins[b].count++;
        }

        std::vector<TriangleBin> lSweep(SAH_K - 1), rSweep(SAH_K - 1);
        lSweep[0] = bins[0];
        rSweep[0] = bins[SAH_K - 1];
        for (int i = 1; i < SAH_K - 1; i++) {
            lSweep[i].aabbMax = glm::max(lSweep[i-1].aabbMax, bins[i].aabbMax);
            lSweep[i].aabbMin = glm::min(lSweep[i-1].aabbMin, bins[i].aabbMin);
            lSweep[i].count   = lSweep[i-1].count + bins[i].count;
            rSweep[i].aabbMax = glm::max(rSweep[i-1].aabbMax, bins[SAH_K-1-i].aabbMax);
            rSweep[i].aabbMin = glm::min(rSweep[i-1].aabbMin, bins[SAH_K-1-i].aabbMin);
            rSweep[i].count   = rSweep[i-1].count + bins[SAH_K-1-i].count;
        }

        for (int i = 0; i < SAH_K - 1; i++) {
            float cost = SurfaceAreaHeuristic(lSweep[i].aabbMin, lSweep[i].aabbMax) * lSweep[i].count
                       + SurfaceAreaHeuristic(rSweep[SAH_K-2-i].aabbMin, rSweep[SAH_K-2-i].aabbMax) * rSweep[SAH_K-2-i].count;
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
            int b = std::clamp(int((triBounds[tri].centroid[bestAxis] - centroidMin[bestAxis]) * bestScale), 0, SAH_K - 1);
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

bool drawMeshAssetUI(MeshAsset& mesh) {
    bool updated = false;
    std::string name = mesh.getName();
    name.resize(128);

    ImGui::PushItemWidth(-FLT_MIN);
    ImGui::Text("Name:");
    if (ImGui::InputText("##Name", name.data(), 128)) {
        mesh.setName(name);
        updated = true;
    }
    ImGui::PopItemWidth();
    
    if (!mesh.getPath().empty()) {
        ImGui::Text("Path:");
        ImGui::SameLine();
        ImGui::TextDisabled("%s", mesh.getPath().c_str());
    }

    ImGui::Text("Vertices: %zu", mesh.getVertices().size());
    ImGui::Text("Faces:    %zu", mesh.getIndices().size() / 3);

    ImGui::Separator();

    bool smooth = mesh.getSmoothShading();
    if (ImGui::Checkbox("Smooth Shading", &smooth)) {
        mesh.setSmoothShading(smooth);
        updated = true;
    }

    ImGui::Separator();

    float ratio = mesh.getSimplifyRatio();
    ImGui::Text("Simplify Ratio:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::SliderFloat("##SimplifyRatio", &ratio, 0.05f, 1.0f, "%.2f")) {
        updated |= mesh.setSimplifyRatio(ratio);
    }
    ImGui::PopItemWidth();
    if (ImGui::BeginTable("##SimplifyButtons", 2, ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        if (ImGui::Button("Apply", ImVec2(-FLT_MIN, 0.0f))) {
            updated |= mesh.applySimplification();
        }
        ImGui::TableSetColumnIndex(1);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.25f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
        if (ImGui::Button("Revert", ImVec2(-FLT_MIN, 0.0f))) {
            updated |= mesh.revertSimplification();
        }
        ImGui::PopStyleColor(3);
        ImGui::EndTable();
    }

    return updated;
}
