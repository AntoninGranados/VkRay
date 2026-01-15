#include "mesh.hpp"

#include <cmath>
#include <limits>

Mesh::Mesh(std::string name, std::vector<Vertex> vertices, std::vector<unsigned int> indices, glm::mat4 transform, MaterialHandle materialHandle):
    Object(name), vertices(vertices), indices(indices), transform(transform), materialHandle(materialHandle) {
    aabbMin = glm::vec3(std::numeric_limits<float>::infinity());
    aabbMax = glm::vec3(-std::numeric_limits<float>::infinity());
    for (const Vertex &v : vertices) {
        aabbMin = glm::min(aabbMin, v.position);
        aabbMax = glm::max(aabbMax, v.position);
    }
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
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(transform[3]));
    glm::mat4 delta(1.0f);
    if (ImGuizmo::Manipulate(
        glm::value_ptr(view),
        glm::value_ptr(proj),
        ImGuizmo::OPERATION::TRANSLATE | ImGuizmo::OPERATION::ROTATE | ImGuizmo::OPERATION::SCALE,
        ImGuizmo::MODE::WORLD,
        glm::value_ptr(model),
        glm::value_ptr(delta)
    )) {
        if (isInvalid(model) || isInvalid(delta)) return false;
        transform = delta * transform;
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
    mesh.aabbMin = aabbMin;
    mesh.aabbMax = aabbMax;
    mesh.indexOffset = -1;  // Computed by the scene
    mesh.triangleCount = indices.size() / 3;
    mesh.materialHandle = materialHandle;
    return mesh;
}
