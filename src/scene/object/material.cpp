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

bool drawGgxMetalUI(Material &material) {
    bool updated = false;
    ImGui::PushItemWidth(-FLT_MIN);

    ImGui::Text("Albedo:");
    if (ImGui::ColorEdit3("##Mat Albedo", glm::value_ptr(material.albedo)))
        updated = true;

    ImGui::Text("Roughness:");
    if (ImGui::DragFloat("##Mat Roughness", &ggxMetalFuzz(material), 0.01, 0.0, 1.0))
        updated = true;
        
    ImGui::PopItemWidth();
    return updated;
}

bool drawGgxPlasticUI(Material &material) {
    bool updated = false;
    ImGui::PushItemWidth(-FLT_MIN);

    ImGui::Text("Albedo:");
    if (ImGui::ColorEdit3("##Mat Albedo", glm::value_ptr(material.albedo)))
        updated = true;

    ImGui::Text("Roughness:");
    if (ImGui::DragFloat("##Mat Roughness", &ggxPlasticRoughness(material), 0.01, 0.0, 1.0))
        updated = true;
    
    ImGui::Text("IoR:");
    if (ImGui::DragFloat("##Mat IoR", &ggxPlasticIoR(material), 0.01, 0.0, FLT_MAX))
        updated = true;
        
    ImGui::PopItemWidth();
    return updated;
}

bool drawMaterialUI(Material &material) {
    bool updated = false;

    material.name.resize(128);

    ImGui::PushItemWidth(-FLT_MIN);
    ImGui::Text("Name:");
    
    ImGui::InputText("##Name", material.name.data(), 128);
    ImGui::PopItemWidth();
    
    const char *types[] = { "Lambertian", "Emissive", "GGX Metal", "GGX Plastic" };
    ImGui::Text("Type:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::Combo("##Mat Type", (int*)&material.type, types, IM_ARRAYSIZE(types)))
        updated = true;
    ImGui::PopItemWidth();
    
    switch (material.type) {
        case MaterialType::Lambertian:    updated |= drawLambertianUI(material);    break;
        case MaterialType::Emissive:      updated |= drawEmissiveUI(material);      break;
        case MaterialType::GgxMetal:      updated |= drawGgxMetalUI(material);      break;
        case MaterialType::GgxPlastic:    updated |= drawGgxPlasticUI(material);    break;
    }

    return updated;
}
