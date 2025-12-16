#include "sphere.hpp"

Sphere::Sphere(std::string name, glm::vec3 center, float radius, MaterialHandle materialHandle):
    Object(name), center(center), radius(radius), materialHandle(materialHandle) {
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
        if (isInvalid(model)) return false;

        glm::vec3 translation, rotation, scale;
        ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(model), glm::value_ptr(translation), glm::value_ptr(rotation), glm::value_ptr(scale));
        const float maxStep = maxStepPerFrame(MAX_GIZMO_LINEAR_SPEED);
        const float maxScaleStep = maxStepPerFrame(MAX_GIZMO_SCALE_SPEED);

        glm::vec3 delta = translation - center;
        center += clampVecDelta(delta, maxStep);

        float targetRadius = glm::max(scale.x, 0.1f);
        radius += clampScalarDelta(targetRadius - radius, maxScaleStep);
        radius = glm::max(radius, 0.1f);

        return true;
    }
    return false;
}

bool Sphere::drawUI(std::vector<Material> &materials) {
    bool updated = false;
    
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
    
    updated |= drawMaterialUI(materials[materialHandle]);
    return updated;
}

GpuSphere Sphere::getStruct() {
    sphere.center = center;
    sphere.radius = radius;
    sphere.materialHandle = materialHandle;
    return sphere;
}
