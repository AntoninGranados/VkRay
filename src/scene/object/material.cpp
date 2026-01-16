#include "material.hpp"

bool drawLambertianUI(Material &material) {
    bool updated = false;

    ImGui::Text("Albedo:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::ColorEdit3("##Mat Albedo", glm::value_ptr(material.albedo)))
        updated = true;
    ImGui::PopItemWidth();

    return updated;
}

bool drawMetalUI(Material &material) {
    bool updated = false;

    ImGui::Text("Albedo:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::ColorEdit3("##Mat Albedo", glm::value_ptr(material.albedo)))
        updated = true;
    ImGui::PopItemWidth();

    ImGui::Text("Fuzz:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::DragFloat("##Mat Fuzz", &metalFuzz(material), 0.01, 0.0, 1.0))
        updated = true;
    ImGui::PopItemWidth();

    return updated;
}

bool drawDielectricUI(Material &material) {
    bool updated = false;

    ImGui::Text("Albedo:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::ColorEdit3("##Mat Albedo", glm::value_ptr(material.albedo)))
        updated = true;
    ImGui::PopItemWidth();

    ImGui::Text("IoR:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::DragFloat("##Mat IoR", &dielectricIoR(material), 0.001, 0.01, 10.0, "%.3f"))
        updated = true;
    ImGui::PopItemWidth();
    
    ImGui::Text("Fuzz:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::DragFloat("##Mat Fuzz", &dielectricFuzz(material), 0.001, 0.0, 1.0, "%.3f"))
        updated = true;
    ImGui::PopItemWidth();

    return updated;
}

bool drawEmissiveUI(Material &material) {
    bool updated = false;

    ImGui::Text("Albedo:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::ColorEdit3("##Mat Albedo", glm::value_ptr(material.albedo)))
        updated = true;
    ImGui::PopItemWidth();

    ImGui::Text("Intensity:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::DragFloat("##Mat Intensity", &emissiveIntensity(material), 0.1, 0.0, 100.0))
        updated = true;
    ImGui::PopItemWidth();

    return updated;
}

bool drawGlossyUI(Material &material) {
    bool updated = false;

    ImGui::Text("Albedo:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::ColorEdit3("##Mat Albedo", glm::value_ptr(material.albedo)))
        updated = true;
    ImGui::PopItemWidth();

    ImGui::Text("IoR:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::DragFloat("##Mat IoR", &glossyIoR(material), 0.001, 0.01, 10.0, "%.3f"))
        updated = true;
    ImGui::PopItemWidth();

    ImGui::Text("Fuzz:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::DragFloat("##Mat Fuzz", &glossyFuzz(material), 0.01, 0.0, 1.0))
        updated = true;
    ImGui::PopItemWidth();

    return updated;
} 

bool drawCheckerboardUI(Material &material) {
    bool updated = false;

    ImGui::Text("Scale:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::DragFloat("##Mat Scale", &checkerboardScale(material), 0.01, 0.01, 10.0, "%.3f"))
        updated = true;
    ImGui::PopItemWidth();

    return updated;
}

bool drawMaterialUI(Material &material) {
    bool updated = false;

    ImGui::SeparatorText("Material");
    const char *types[] = { "Lambertian", "Metal", "Dielectric", "Emissive", "Glossy", "Checkerboard" };
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::Combo("##Mat Type", (int*)&material.type, types, IM_ARRAYSIZE(types)))
        updated = true;
    ImGui::PopItemWidth();
    
    switch (material.type) {
        case MaterialType::Lambertian:   updated |= drawLambertianUI(material);   break;
        case MaterialType::Metal:        updated |= drawMetalUI(material);        break;
        case MaterialType::Dielectric:   updated |= drawDielectricUI(material);   break;
        case MaterialType::Emissive:     updated |= drawEmissiveUI(material);     break;
        case MaterialType::Glossy:       updated |= drawGlossyUI(material);       break;
        case MaterialType::Checkerboard: updated |= drawCheckerboardUI(material); break;
    }

    return updated;
}
