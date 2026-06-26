#include "material.hpp"

#include "imgui/imgui.h"

bool drawPrincipledUI(Material& material) {
    bool updated = false;
    ImGui::PushItemWidth(-FLT_MIN);

    ImGui::Text("Albedo:");
    if (ImGui::ColorEdit3("##Mat Albedo", glm::value_ptr(material.albedo)))
        updated = true;

    ImGui::Text("Roughness:");
    if (ImGui::DragFloat("##Mat Roughness", &material.roughness, 0.01f, 0.0f, 1.0f))
        updated = true;

    ImGui::Text("Metalness:");
    if (ImGui::DragFloat("##Mat Metalness", &material.metalness, 0.01f, 0.0f, 1.0f))
        updated = true;

    ImGui::Text("IoR:");
    if (ImGui::DragFloat("##Mat IoR", &material.ior, 0.01f, 1.0f, FLT_MAX))
        updated = true;

    ImGui::Text("Transmission:");
    if (ImGui::DragFloat("##Mat Transmission", &material.transmission, 0.01f, 0.0f, 1.0f))
        updated = true;

    if (material.transmission > 0.0f) {
        ImGui::Text("Density:");
        if (ImGui::DragFloat("##Mat Density", &material.density, 0.01f, 0.0f, FLT_MAX))
            updated = true;
    }

    ImGui::PopItemWidth();
    return updated;
}

bool drawEmissiveUI(Material& material) {
    bool updated = false;
    ImGui::PushItemWidth(-FLT_MIN);

    ImGui::Text("Albedo:");
    if (ImGui::ColorEdit3("##Mat Albedo", glm::value_ptr(material.albedo)))
        updated = true;

    ImGui::Text("Intensity:");
    if (ImGui::DragFloat("##Mat Intensity", &material.emissionStrength, 0.1f, 0.0f, 100.0f))
        updated = true;

    ImGui::PopItemWidth();
    return updated;
}

bool drawLambertianUI(Material& material) {
    bool updated = false;
    ImGui::PushItemWidth(-FLT_MIN);

    ImGui::Text("Albedo:");
    if (ImGui::ColorEdit3("##Mat Albedo", glm::value_ptr(material.albedo)))
        updated = true;

    ImGui::PopItemWidth();
    return updated;
}

bool drawGgxMetalUI(Material& material) {
    bool updated = false;
    ImGui::PushItemWidth(-FLT_MIN);

    ImGui::Text("Albedo:");
    if (ImGui::ColorEdit3("##Mat Albedo", glm::value_ptr(material.albedo)))
        updated = true;

    ImGui::Text("Roughness:");
    if (ImGui::DragFloat("##Mat Roughness", &material.roughness, 0.01f, 0.0f, 1.0f))
        updated = true;

    ImGui::PopItemWidth();
    return updated;
}

bool drawGgxGlossyUI(Material& material) {
    bool updated = false;
    ImGui::PushItemWidth(-FLT_MIN);

    ImGui::Text("Albedo:");
    if (ImGui::ColorEdit3("##Mat Albedo", glm::value_ptr(material.albedo)))
        updated = true;

    ImGui::Text("Roughness:");
    if (ImGui::DragFloat("##Mat Roughness", &material.roughness, 0.01f, 0.0f, 1.0f))
        updated = true;

    ImGui::Text("IoR:");
    if (ImGui::DragFloat("##Mat IoR", &material.ior, 0.01f, 1.0f, FLT_MAX))
        updated = true;

    ImGui::PopItemWidth();
    return updated;
}

bool drawDielectricUI(Material& material) {
    bool updated = false;
    ImGui::PushItemWidth(-FLT_MIN);

    ImGui::Text("Albedo:");
    if (ImGui::ColorEdit3("##Mat Albedo", glm::value_ptr(material.albedo)))
        updated = true;

    ImGui::Text("IoR:");
    if (ImGui::DragFloat("##Mat IoR", &material.ior, 0.01f, 1.0f, FLT_MAX))
        updated = true;

    ImGui::Text("Roughness:");
    if (ImGui::DragFloat("##Mat Roughness", &material.roughness, 0.01f, 0.0f, 1.0f))
        updated = true;

    ImGui::Text("Density:");
    if (ImGui::DragFloat("##Mat Density", &material.density, 0.01f, 0.0f, FLT_MAX))
        updated = true;

    ImGui::PopItemWidth();
    return updated;
}

bool drawVolumeUI(Material& material) {
    bool updated = false;
    ImGui::PushItemWidth(-FLT_MIN);

    ImGui::Text("Albedo:");
    if (ImGui::ColorEdit3("##Mat Albedo", glm::value_ptr(material.albedo)))
        updated = true;

    ImGui::Text("Density:");
    if (ImGui::DragFloat("##Mat Density", &material.density, 0.01f, 0.0f, FLT_MAX))
        updated = true;

    ImGui::Text("Anisotropic:");
    if (ImGui::DragFloat("##Mat Anisotropic", &material.anisotropic, 0.01f, -1.0f, 1.0))
        updated = true;

    ImGui::PopItemWidth();
    return updated;
}

bool drawProgrammableUI(Material& material) {
    bool updated = false;
    ImGui::PushItemWidth(-FLT_MIN);

    ImGui::Text("Albedo:");
    if (ImGui::ColorEdit3("##Mat Albedo", glm::value_ptr(material.albedo)))
        updated = true;

    ImGui::PopItemWidth();
    return updated;
}

bool drawMaterialUI(Material& material) {
    bool updated = false;
    MaterialType prevType = material.type;

    material.name.resize(128);

    ImGui::PushItemWidth(-FLT_MIN);

    ImGui::Text("Name:");
    ImGui::InputText("##Name", material.name.data(), 128);

    const char* types[] = { "Principled", "Emissive", "Lambertian", "GGX Metal", "GGX Glossy", "Dielectric", "Volume", "Programmable" };
    ImGui::Text("Type:");
    if (ImGui::Combo("##Mat Type", (int*)&material.type, types, IM_ARRAYSIZE(types)))
        updated = true;

    ImGui::PopItemWidth();

    if (material.type != prevType) {
        switch (material.type) {
            case MaterialType::Principled:
                material.roughness    = 0.3f;
                material.metalness    = 0.0f;
                material.ior          = 1.5f;
                material.transmission = 0.0f;
                break;
            case MaterialType::Emissive:
                material.emissionStrength = 1.0f;
                break;
            case MaterialType::GgxMetal:
                material.roughness = 0.2f;
                break;
            case MaterialType::GgxGlossy:
                material.roughness = 0.2f;
                material.ior       = 1.5f;
                break;
            case MaterialType::Dielectric:
                material.albedo    = glm::vec3(1.0f);
                material.roughness = 0.0f;
                material.ior       = 1.5f;
                break;
            default:
                break;
        }
    }

    switch (material.type) {
        case MaterialType::Principled:   updated |= drawPrincipledUI(material);   break;
        case MaterialType::Emissive:     updated |= drawEmissiveUI(material);     break;
        case MaterialType::Lambertian:   updated |= drawLambertianUI(material);   break;
        case MaterialType::GgxMetal:     updated |= drawGgxMetalUI(material);     break;
        case MaterialType::GgxGlossy:    updated |= drawGgxGlossyUI(material);    break;
        case MaterialType::Dielectric:   updated |= drawDielectricUI(material);   break;
        case MaterialType::Volume:       updated |= drawVolumeUI(material);       break;
        case MaterialType::Programmable: updated |= drawProgrammableUI(material); break;
    }

    return updated;
}
