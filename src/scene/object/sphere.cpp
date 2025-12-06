#include "sphere.hpp"

Sphere::Sphere(std::string name, glm::vec3 center, float radius, Material mat):
    name(name), center(center), radius(radius), mat(mat) {
}

float Sphere::rayIntersection(const Ray &ray) {
    const glm::vec3 p = center - ray.origin;
    const float dp = glm::dot(ray.dir, p);
    const float c = glm::dot(p, p) - radius * radius;
    const float delta = dp * dp - c;
    if (delta < 0.0f) return -1.0f;

    const float sqrt_delta = std::sqrt(delta);

    float t = dp - sqrt_delta;
    if (t >= 0.0f) return t;

    t = dp + sqrt_delta;
    return t >= 0.0f ? t : -1.0f;
}

bool Sphere::drawGuizmo(const glm::mat4 &view, const glm::mat4 &proj) {
    glm::mat4 model = glm::translate(glm::mat4(1.0), center);
    model = glm::scale(model, glm::vec3(radius));

    if (ImGuizmo::Manipulate(
        glm::value_ptr(view),
        glm::value_ptr(proj),
        ImGuizmo::OPERATION::SCALE_X | ImGuizmo::OPERATION::TRANSLATE,
        ImGuizmo::MODE::WORLD, 
        glm::value_ptr(model)
    )) {
        if (isnan4x4(model)) return false;

        glm::vec3 translation, rotation, scale;
        ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(model), glm::value_ptr(translation), glm::value_ptr(rotation), glm::value_ptr(scale));
        center = translation;
        radius = glm::max(scale.x, 0.1f);

        return true;
    }
    return false;
}

bool Sphere::drawUI() {
    bool updated = false;

    char buff[128];
    memcpy(buff, name.data(), name.size());
    ImGui::Text("Name:");
    ImGui::InputText("##Name", buff, 128);
    name = std::string(buff);
    
    ImGui::Text("Position:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::DragFloat3("##Position", glm::value_ptr(center), 0.01))
        updated = true;
    ImGui::PopItemWidth();
    
    ImGui::Text("Radius:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::DragFloat("##Radius", &radius, 0.01, 0.0))
        updated = true;
    ImGui::PopItemWidth();
    
    updated |= drawMaterialUI(mat);
    return updated;
}

GpuSphere Sphere::getStruct() {
    sphere.center = center;
    sphere.radius = radius;
    sphere.mat = mat;
    return sphere;
}
