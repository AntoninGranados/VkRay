#include "mesh.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

Mesh::Mesh(std::string name, std::vector<Vertex> vertices, std::vector<unsigned int> indices, glm::mat4 transform, MaterialHandle materialHandle):
    Object(name), vertices(vertices), indices(indices), transform(transform), materialHandle(materialHandle) {
    buildBvh();
}


float Mesh::rayIntersection(const Ray &ray) {
    glm::mat4 invTransform = glm::inverse(transform);
    glm::vec3 localOrigin = glm::vec3(invTransform * glm::vec4(ray.origin, 1.0f));
    glm::vec3 localDir = glm::vec3(invTransform * glm::vec4(ray.dir, 0.0f));
    Ray localRay{ localOrigin, localDir };

    float tClosest = std::numeric_limits<float>::infinity();
    bool hit = false;
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        const glm::vec3 v0 = glm::vec3(vertices[indices[i + 0]].position);
        const glm::vec3 v1 = glm::vec3(vertices[indices[i + 1]].position);
        const glm::vec3 v2 = glm::vec3(vertices[indices[i + 2]].position);

        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 pvec = glm::cross(localRay.dir, edge2);
        float det = glm::dot(edge1, pvec);
        if (std::fabs(det) < 1e-6f)
            continue;

        float invDet = 1.0f / det;
        glm::vec3 tvec = localRay.origin - v0;
        float u = glm::dot(tvec, pvec) * invDet;
        if (u < 0.0f || u > 1.0f)
            continue;

        glm::vec3 qvec = glm::cross(tvec, edge1);
        float v = glm::dot(localRay.dir, qvec) * invDet;
        if (v < 0.0f || (u + v) > 1.0f)
            continue;

        float t = glm::dot(edge2, qvec) * invDet;
        if (t >= 0.0f && t < tClosest) {
            tClosest = t;
            hit = true;
        }
    }
    return hit ? tClosest : -1.0f;
}


bool Mesh::drawGuizmo(const glm::mat4 &view, const glm::mat4 &proj) {
    glm::vec3 currentPos, currentRot, currentScale;
    ImGuizmo::DecomposeMatrixToComponents(
        glm::value_ptr(transform),
        glm::value_ptr(currentPos),
        glm::value_ptr(currentRot),
        glm::value_ptr(currentScale)
    );

    glm::mat4 model = transform;
    if (ImGuizmo::Manipulate(
        glm::value_ptr(view),
        glm::value_ptr(proj),
        ImGuizmo::OPERATION::TRANSLATE | ImGuizmo::OPERATION::ROTATE | ImGuizmo::OPERATION::SCALE,
        ImGuizmo::MODE::WORLD,
        glm::value_ptr(model)
    )) {
        if (isInvalid(model)) return false;

        glm::vec3 targetPos, targetRot, targetScale;
        ImGuizmo::DecomposeMatrixToComponents(
            glm::value_ptr(model),
            glm::value_ptr(targetPos),
            glm::value_ptr(targetRot),
            glm::value_ptr(targetScale)
        );

        const float maxStep = maxStepPerFrame(MAX_GIZMO_LINEAR_SPEED);
        targetPos = currentPos + clampVecDelta(targetPos - currentPos, maxStep);

        const float scaleSpeed = MAX_GIZMO_SCALE_SPEED * 0.2f;
        const float maxScaleStep = maxStepPerFrame(scaleSpeed);
        targetScale = currentScale + clampVecDeltaPerAxis(targetScale - currentScale, maxScaleStep);
        targetScale = glm::max(targetScale, glm::vec3(0.001f));

        ImGuizmo::RecomposeMatrixFromComponents(
            glm::value_ptr(targetPos),
            glm::value_ptr(targetRot),
            glm::value_ptr(targetScale),
            glm::value_ptr(transform)
        );
        return true;
    }

    return false;
}

bool Mesh::drawUI(std::vector<Material> &materials) {
    bool updated = false;

    glm::vec3 translation, rotation, scale;
    ImGuizmo::DecomposeMatrixToComponents(
        glm::value_ptr(transform),
        glm::value_ptr(translation),
        glm::value_ptr(rotation),
        glm::value_ptr(scale)
    );

    ImGui::Text("Position:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::DragFloat3("##Position", glm::value_ptr(translation), 0.01f))
        updated = true;
    ImGui::PopItemWidth();

    ImGui::Text("Rotation:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::DragFloat3("##Rotation", glm::value_ptr(rotation), 0.1f))
        updated = true;
    ImGui::PopItemWidth();

    ImGui::Text("Scale:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::DragFloat3("##Scale", glm::value_ptr(scale), 0.01f, 0.001f, 1000.0f))
        updated = true;
    ImGui::PopItemWidth();

    if (updated) {
        ImGuizmo::RecomposeMatrixFromComponents(
            glm::value_ptr(translation),
            glm::value_ptr(rotation),
            glm::value_ptr(scale),
            glm::value_ptr(transform)
        );
    }

    updated |= drawMaterialUI(materials[materialHandle]);
    return updated;
}

float Mesh::getArea() {
    float area = 0.0f;
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        const glm::vec3 v0 = glm::vec3(vertices[indices[i + 0]].position);
        const glm::vec3 v1 = glm::vec3(vertices[indices[i + 1]].position);
        const glm::vec3 v2 = glm::vec3(vertices[indices[i + 2]].position);
        area += 0.5f * glm::length(glm::cross(v1 - v0, v2 - v0));
    }
    return area;
}

GpuMesh Mesh::getStruct() {
    mesh.transform = transform;
    mesh.invTransform = glm::inverse(transform);
    mesh.indexOffset = -1;  // Computed by the scene
    mesh.triangleCount = indices.size() / 3;
    mesh.bvhOffset = 0;     // Computed by the scene
    mesh.bvhNodeCount = static_cast<uint32_t>(bvhNodes.size());
    mesh.materialHandle = materialHandle;
    return mesh;
}


size_t Mesh::buildBvhNode(std::vector<TriBounds> &triBounds, std::vector<uint32_t> &triIndices, uint32_t start, uint32_t count) {
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

void Mesh::buildBvh() {
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
