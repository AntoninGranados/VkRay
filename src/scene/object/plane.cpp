#include "plane.hpp"

Plane::Plane(std::string name, glm::vec3 point, glm::vec3 normal, Material mat):
    name(name), point(point), normal(normal), mat(mat) {
}

float Plane::rayIntersection(const Ray &ray) {
    const float denom = glm::dot(normal, ray.dir);
    if (std::abs(denom) <= 1e-6f) return -1.0f;

    const float t = glm::dot(point - ray.origin, normal) / denom;
    return t >= 0.0f ? t : -1.0f;
}

void Plane::drawGuizmo(int &frameCount, const glm::mat4 &view, const glm::mat4 &proj) {
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
        if (isnan4x4(model)) return;

        point = glm::vec3(model[3]);
        normal = glm::normalize(glm::vec3(model[2]));
        
        frameCount = 0;
    }
}

void Plane::drawUI(int &frameCount) {
    char buff[128];
    memcpy(buff, name.data(), name.size());
    ImGui::Text("Name:");
    ImGui::InputText("##Name", buff, 128);
    name = std::string(buff);
    
    ImGui::Text("Point:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::DragFloat3("##Point", glm::value_ptr(point), 0.01)) frameCount = 0;
    ImGui::PopItemWidth();
    
    ImGui::Text("Normal:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::DragFloat3("##Normal", glm::value_ptr(normal), 0.01)) frameCount = 0;
    ImGui::PopItemWidth();
    normal = glm::normalize(normal);
    
    drawMaterialUI(frameCount, mat);
}

GpuPlane Plane::getStruct() {
    plane.point = point;
    plane.normal = normal;
    plane.mat = mat;
    return plane;
}
