#include "object.hpp"

Ray getRay(const glm::vec2 &mousePos, const glm::vec2 &screenSize, const Camera &camera) {
    const float invWidth = 1.0f / screenSize.x;
    const float invHeight = 1.0f / screenSize.y;

    const glm::vec3 forward = glm::normalize(camera.getDirection());
    const glm::vec3 right = glm::normalize(glm::cross(forward, camera.getUp()));
    const glm::vec3 up = glm::cross(right, forward);

    const float ndcX = mousePos.x * 2.0f * invWidth - 1.0f;
    const float ndcY = 1.0f - mousePos.y * 2.0f * invHeight;

    const float tanHFov = camera.getTanHFov();
    const float aspect = screenSize.x * invHeight;

    const float camX = ndcX * aspect * tanHFov;
    const float camY = ndcY * tanHFov;

    glm::vec3 dir = glm::normalize(camX * right + camY * up + forward);
    return Ray{ camera.getPosition(), dir };
}

bool drawMaterialUI(Material &mat) {
    bool updated = false;

    ImGui::SeparatorText("Material");
    const char *types[5] = { "Lambertian", "Metal", "Dielectric", "Emissive", "Animated" };
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::Combo("##Mat-type", (int*)&mat.type, types, IM_ARRAYSIZE(types)))
        updated = true;
    ImGui::PopItemWidth();
    
    if (mat.type == MaterialType::Animated)
        return updated;

    ImGui::Text("Albedo:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::ColorEdit3("##Mat-albedo", glm::value_ptr(mat.albedo)))
        updated = true;
    ImGui::PopItemWidth();
    
    switch (mat.type) {
        case MaterialType::Lambertian: break;
        case MaterialType::Metal: {
            ImGui::Text("Fuzz:");
            ImGui::PushItemWidth(-FLT_MIN);
            if (ImGui::DragFloat("##Mat-fuzz", &mat.fuzz, 0.01, 0.0, 1.0))
                updated = true;
            ImGui::PopItemWidth();
        } break;
        case MaterialType::Dielectric: {
            ImGui::Text("Refraction Index:");
            ImGui::PushItemWidth(-FLT_MIN);
            if (ImGui::DragFloat("##Mat-index", &mat.refraction_index, 0.001, 0.01, 10.0, "%.3f"))
                updated = true;
            ImGui::PopItemWidth();
        } break;
        case MaterialType::Emissive: {
            ImGui::Text("Intensity:");
            ImGui::PushItemWidth(-FLT_MIN);
            if (ImGui::DragFloat("##Mat-intensity", &mat.intensity, 0.1, 0.0, 100.0))
                updated = true;
            ImGui::PopItemWidth();
        } break;
        default: break;
    }

    return updated;
}

bool isInvalid(glm::mat4 mat) {
    bool invalid = false;
    for (size_t i = 0; i < 4; i++) {
        const glm::vec4 col = mat[i];
        invalid |= glm::any(glm::isnan(col)) || glm::any(glm::isinf(col));
    }
    return invalid;
}
