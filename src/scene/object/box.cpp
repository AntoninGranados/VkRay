#include "box.hpp"

#include <cmath>
#include <limits>

Box::Box(std::string name, glm::vec3 cornerMin, glm::vec3 cornerMax, Material mat):
    name(name), cornerMin(cornerMin), cornerMax(cornerMax), mat(mat) {
}

float Box::rayIntersection(const Ray &ray) {
    float tmin = -std::numeric_limits<float>::infinity();
    float tmax = std::numeric_limits<float>::infinity();

    for (int i = 0; i < 3; ++i) {
        const float dir = ray.dir[i];
        if (std::abs(dir) < 1e-8f) {
            if (ray.origin[i] < cornerMin[i] || ray.origin[i] > cornerMax[i]) return -1.0f;
            continue;
        }

        const float invD = 1.0f / dir;
        float t0 = (cornerMin[i] - ray.origin[i]) * invD;
        float t1 = (cornerMax[i] - ray.origin[i]) * invD;
        if (invD < 0.0f) std::swap(t0, t1);

        if (t0 > tmin) tmin = t0;
        if (t1 < tmax) tmax = t1;
        if (tmax < tmin) return -1.0f;
    }

    if (tmax < 0.0f) return -1.0f;
    return tmin >= 0.0f ? tmin : tmax;
}

void Box::drawGuizmo(int &frameCount, const glm::mat4 &view, const glm::mat4 &proj) {
    glm::vec3 center = (cornerMax + cornerMin) * glm::vec3(0.5);
    glm::mat4 model = glm::translate(glm::mat4(1.0), center);
    model = glm::scale(model, glm::vec3(cornerMax - cornerMin));

    if (ImGuizmo::Manipulate(
        glm::value_ptr(view),
        glm::value_ptr(proj),
        ImGuizmo::OPERATION::SCALE | ImGuizmo::OPERATION::TRANSLATE,
        ImGuizmo::MODE::WORLD, 
        glm::value_ptr(model)
    )) {
        if (isnan4x4(model)) return;

        glm::vec3 translation, rotation, scale;
        ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(model), glm::value_ptr(translation), glm::value_ptr(rotation), glm::value_ptr(scale));
        const glm::vec3 halfSize = glm::max(scale, glm::vec3(0.1)) * 0.5f;
        cornerMin = translation - halfSize;
        cornerMax = translation + halfSize;
        
        frameCount = 0;
    }
}

void Box::drawUI(int &frameCount) {
    char buff[128];
    memcpy(buff, name.data(), name.size());
    ImGui::Text("Name:");
    ImGui::InputText("##Name", buff, 128);
    name = std::string(buff);
    
    ImGui::Text("Corner Max:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::DragFloat3("##CornerMax", glm::value_ptr(cornerMax), 0.01)) frameCount = 0;
    ImGui::PopItemWidth();

    ImGui::Text("Corner Min:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::DragFloat3("##CornerMin", glm::value_ptr(cornerMin), 0.01)) frameCount = 0;
    ImGui::PopItemWidth();
    
    drawMaterialUI(frameCount, mat);
}

GpuBox Box::getStruct() {
    box.cornerMin = cornerMin;
    box.cornerMax = cornerMax;
    box.mat = mat;
    return box;
}
