#include "material.hpp"

bool drawLambertianUI(Material &mat) {
    bool updated = false;

    ImGui::Text("Albedo:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::ColorEdit3("##Mat-albedo", glm::value_ptr(mat.albedo)))
        updated = true;
    ImGui::PopItemWidth();

    return updated;
}

bool drawMetalUI(Material &mat) {
    bool updated = false;

    ImGui::Text("Albedo:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::ColorEdit3("##Mat-albedo", glm::value_ptr(mat.albedo)))
        updated = true;
    ImGui::PopItemWidth();

    ImGui::Text("Fuzz:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::DragFloat("##Mat-fuzz", &mat.fuzz, 0.01, 0.0, 1.0))
        updated = true;
    ImGui::PopItemWidth();

    return updated;
}

bool drawDielectricUI(Material &mat) {
    bool updated = false;

    ImGui::Text("Albedo:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::ColorEdit3("##Mat-albedo", glm::value_ptr(mat.albedo)))
        updated = true;
    ImGui::PopItemWidth();

    ImGui::Text("IoR:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::DragFloat("##Mat-index", &mat.refraction_index, 0.001, 0.01, 10.0, "%.3f"))
        updated = true;
    ImGui::PopItemWidth();

    return updated;
}

bool drawEmissiveUI(Material &mat) {
    bool updated = false;

    ImGui::Text("Albedo:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::ColorEdit3("##Mat-albedo", glm::value_ptr(mat.albedo)))
        updated = true;
    ImGui::PopItemWidth();

    ImGui::Text("Intensity:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::DragFloat("##Mat-intensity", &mat.intensity, 0.1, 0.0, 100.0))
        updated = true;
    ImGui::PopItemWidth();

    return updated;
}

bool drawGlossyUI(Material &mat) {
    bool updated = false;

    ImGui::Text("Albedo:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::ColorEdit3("##Mat-albedo", glm::value_ptr(mat.albedo)))
        updated = true;
    ImGui::PopItemWidth();

    ImGui::Text("IoR:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::DragFloat("##Mat-index", &mat.refraction_index, 0.001, 0.01, 10.0, "%.3f"))
        updated = true;
    ImGui::PopItemWidth();

    ImGui::Text("Fuzz:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::DragFloat("##Mat-fuzz", &mat.fuzz, 0.01, 0.0, 1.0))
        updated = true;
    ImGui::PopItemWidth();

    return updated;
} 

bool drawCheckerboardUI(Material &mat) {
    return false;
}

bool drawMaterialUI(Material &mat) {
    bool updated = false;

    ImGui::SeparatorText("Material");
    const char *types[] = { "Lambertian", "Metal", "Dielectric", "Emissive", "Glossy", "Checkerboard" };
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::Combo("##Mat-type", (int*)&mat.type, types, IM_ARRAYSIZE(types)))
        updated = true;
    ImGui::PopItemWidth();
    
    switch (mat.type) {
        case MaterialType::Lambertian:   updated |= drawLambertianUI(mat);   break;
        case MaterialType::Metal:        updated |= drawMetalUI(mat);        break;
        case MaterialType::Dielectric:   updated |= drawDielectricUI(mat);   break;
        case MaterialType::Emissive:     updated |= drawEmissiveUI(mat);     break;
        case MaterialType::Glossy:       updated |= drawGlossyUI(mat);       break;
        case MaterialType::Checkerboard: updated |= drawCheckerboardUI(mat); break;
    }

    return updated;
}
