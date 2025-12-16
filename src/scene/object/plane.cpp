#include <cmath>

#include "plane.hpp"

Plane::Plane(std::string name, glm::vec3 point, glm::vec3 normal, MaterialHandle materialHandle):
    Object(name), point(point), normal(normal), materialHandle(materialHandle) {
}

float Plane::rayIntersection(const Ray &ray) {
    const float denom = glm::dot(normal, ray.dir);
    if (std::abs(denom) <= 1e-6f) return -1.0f;

    const float t = glm::dot(point - ray.origin, normal) / denom;
    return t >= 0.0f ? t : -1.0f;
}

bool Plane::drawGuizmo(const glm::mat4 &view, const glm::mat4 &proj) {
    const glm::vec3 up = glm::abs(normal.z) < 0.999f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 tangent = glm::normalize(glm::cross(up, normal));
    const glm::vec3 bitangent = glm::cross(normal, tangent);

    glm::mat4 model(1.0f);
    model[0] = glm::vec4(tangent, 0.0f);
    model[1] = glm::vec4(bitangent, 0.0f);
    model[2] = glm::vec4(normal, 0.0f);
    model[3] = glm::vec4(point, 1.0f);

    if (ImGuizmo::Manipulate(
        glm::value_ptr(view),
        glm::value_ptr(proj),
        ImGuizmo::OPERATION::ROTATE | ImGuizmo::OPERATION::TRANSLATE,
        ImGuizmo::MODE::WORLD, 
        glm::value_ptr(model)
    )) {
        if (isInvalid(model)) return false;

        const float maxStep = maxStepPerFrame(MAX_GIZMO_LINEAR_SPEED);
        const float maxAngularStep = maxStepPerFrame(MAX_GIZMO_ANGULAR_SPEED);

        glm::vec3 targetPoint = glm::vec3(model[3]);
        glm::vec3 delta = targetPoint - point;
        point += clampVecDelta(delta, maxStep);

        glm::vec3 targetNormal = glm::normalize(glm::vec3(model[2]));
        float angle = std::acos(glm::clamp(glm::dot(glm::normalize(normal), targetNormal), -1.0f, 1.0f));
        if (angle > maxAngularStep && angle > 1e-5f && maxAngularStep > 0.0f) {
            float t = maxAngularStep / angle;
            targetNormal = glm::normalize(glm::mix(normal, targetNormal, t));
        }
        normal = targetNormal;
        
        return true;
    }
    return false;
}

bool Plane::drawUI(std::vector<Material> &materials) {
    bool updated = false;
    
    ImGui::Text("Point:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::DragFloat3("##Point", glm::value_ptr(point), 0.01))
        updated = true;
    ImGui::PopItemWidth();
    
    ImGui::Text("Normal:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::DragFloat3("##Normal", glm::value_ptr(normal), 0.01))
        updated = true;
    ImGui::PopItemWidth();
    normal = glm::normalize(normal);
    
    updated |= drawMaterialUI(materials[materialHandle]);
    return updated;
}

GpuPlane Plane::getStruct() {
    plane.point = point;
    plane.normal = normal;
    plane.materialHandle = materialHandle;
    return plane;
}
