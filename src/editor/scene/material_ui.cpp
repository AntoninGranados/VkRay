#include "material_ui.hpp"

#include "imgui/imgui.h"

#include "core/core.hpp"
#include "core/scene/scene.hpp"
#include "editor/ui_utils.hpp"

static bool drawPrincipledUI(MaterialHandle handle) {
    Material& material = Core::getScene().getMaterials()[handle];
    bool updated = false;
    ImGui::PushItemWidth(-FLT_MIN);

    ui::drawKeyframeButton(handle, "albedo"); ImGui::Text("Albedo:");
    if (ImGui::ColorEdit3("##Mat Albedo", glm::value_ptr(material.albedo)))
        updated = true;

    ui::drawKeyframeButton(handle, "roughness"); ImGui::Text("Roughness:");
    if (ImGui::SliderFloat("##Mat Roughness", &material.roughness, 0.0f, 1.0f))
        updated = true;

    ui::drawKeyframeButton(handle, "metalness"); ImGui::Text("Metalness:");
    if (ImGui::SliderFloat("##Mat Metalness", &material.metalness, 0.0f, 1.0f))
        updated = true;

    ui::drawKeyframeButton(handle, "ior"); ImGui::Text("IoR:");
    if (ImGui::DragFloat("##Mat IoR", &material.ior, 0.01f, 1.0f, FLT_MAX))
        updated = true;

    ui::drawKeyframeButton(handle, "transmission"); ImGui::Text("Transmission:");
    if (ImGui::SliderFloat("##Mat Transmission", &material.transmission, 0.0f, 1.0f))
        updated = true;

    if (material.transmission > 0.0f) {
        ui::drawKeyframeButton(handle, "density"); ImGui::Text("Density:");
        if (ImGui::DragFloat("##Mat Density", &material.density, 0.01f, 0.0f, FLT_MAX))
            updated = true;
    }

    ImGui::PopItemWidth();
    return updated;
}

static bool drawEmissiveUI(MaterialHandle handle) {
    Material& material = Core::getScene().getMaterials()[handle];
    bool updated = false;
    ImGui::PushItemWidth(-FLT_MIN);

    ui::drawKeyframeButton(handle, "albedo"); ImGui::Text("Albedo:");
    if (ImGui::ColorEdit3("##Mat Albedo", glm::value_ptr(material.albedo)))
        updated = true;

    ui::drawKeyframeButton(handle, "emissionStrength"); ImGui::Text("Intensity:");
    if (ImGui::DragFloat("##Mat Intensity", &material.emissionStrength, 0.1f, 0.0f, FLT_MAX))
        updated = true;

    ImGui::PopItemWidth();
    return updated;
}

static bool drawLambertianUI(MaterialHandle handle) {
    Material& material = Core::getScene().getMaterials()[handle];
    bool updated = false;
    ImGui::PushItemWidth(-FLT_MIN);

    ui::drawKeyframeButton(handle, "albedo"); ImGui::Text("Albedo:");
    if (ImGui::ColorEdit3("##Mat Albedo", glm::value_ptr(material.albedo)))
        updated = true;

    ImGui::PopItemWidth();
    return updated;
}

static bool drawGgxMetalUI(MaterialHandle handle) {
    Material& material = Core::getScene().getMaterials()[handle];
    bool updated = false;
    ImGui::PushItemWidth(-FLT_MIN);

    ui::drawKeyframeButton(handle, "albedo"); ImGui::Text("Albedo:");
    if (ImGui::ColorEdit3("##Mat Albedo", glm::value_ptr(material.albedo)))
        updated = true;

    ui::drawKeyframeButton(handle, "roughness"); ImGui::Text("Roughness:");
    if (ImGui::SliderFloat("##Mat Roughness", &material.roughness, 0.0f, 1.0f))
        updated = true;

    ImGui::PopItemWidth();
    return updated;
}

static bool drawGgxGlossyUI(MaterialHandle handle) {
    Material& material = Core::getScene().getMaterials()[handle];
    bool updated = false;
    ImGui::PushItemWidth(-FLT_MIN);

    ui::drawKeyframeButton(handle, "albedo"); ImGui::Text("Albedo:");
    if (ImGui::ColorEdit3("##Mat Albedo", glm::value_ptr(material.albedo)))
        updated = true;

    ui::drawKeyframeButton(handle, "roughness"); ImGui::Text("Roughness:");
    if (ImGui::SliderFloat("##Mat Roughness", &material.roughness, 0.0f, 1.0f))
        updated = true;

    ui::drawKeyframeButton(handle, "ior"); ImGui::Text("IoR:");
    if (ImGui::DragFloat("##Mat IoR", &material.ior, 0.01f, 1.0f, FLT_MAX))
        updated = true;

    ImGui::PopItemWidth();
    return updated;
}

static bool drawDielectricUI(MaterialHandle handle) {
    Material& material = Core::getScene().getMaterials()[handle];
    bool updated = false;
    ImGui::PushItemWidth(-FLT_MIN);

    ui::drawKeyframeButton(handle, "albedo"); ImGui::Text("Albedo:");
    if (ImGui::ColorEdit3("##Mat Albedo", glm::value_ptr(material.albedo)))
        updated = true;

    ui::drawKeyframeButton(handle, "ior"); ImGui::Text("IoR:");
    if (ImGui::DragFloat("##Mat IoR", &material.ior, 0.01f, 1.0f, FLT_MAX))
        updated = true;

    ui::drawKeyframeButton(handle, "roughness"); ImGui::Text("Roughness:");
    if (ImGui::SliderFloat("##Mat Roughness", &material.roughness, 0.0f, 1.0f))
        updated = true;

    ui::drawKeyframeButton(handle, "density"); ImGui::Text("Density:");
    if (ImGui::DragFloat("##Mat Density", &material.density, 0.01f, 0.0f, FLT_MAX))
        updated = true;

    if (material.density > 0.0f) {
        ui::drawKeyframeButton(handle, "transmission"); ImGui::Text("Scatter albedo:");
        if (ImGui::SliderFloat("##Mat Transmission", &material.transmission, 0.0f, 1.0f))
            updated = true;

        if (material.transmission > 0.0f) {
            ui::drawKeyframeButton(handle, "anisotropic"); ImGui::Text("Anisotropic:");
            if (ImGui::DragFloat("##Mat Anisotropic", &material.anisotropic, 0.01f, -1.0f, 1.0f))
                updated = true;
        }
    }

    ImGui::PopItemWidth();
    return updated;
}

static bool drawVolumeUI(MaterialHandle handle) {
    Material& material = Core::getScene().getMaterials()[handle];
    bool updated = false;
    ImGui::PushItemWidth(-FLT_MIN);

    ui::drawKeyframeButton(handle, "albedo"); ImGui::Text("Albedo:");
    if (ImGui::ColorEdit3("##Mat Albedo", glm::value_ptr(material.albedo)))
        updated = true;

    ui::drawKeyframeButton(handle, "density"); ImGui::Text("Density:");
    if (ImGui::DragFloat("##Mat Density", &material.density, 0.01f, 0.0f, FLT_MAX))
        updated = true;

    ui::drawKeyframeButton(handle, "anisotropic"); ImGui::Text("Anisotropic:");
    if (ImGui::SliderFloat("##Mat Anisotropic", &material.anisotropic, -1.0f, 1.0))
        updated = true;

    ImGui::PopItemWidth();
    return updated;
}

static bool drawProgrammableUI(MaterialHandle handle) {
    Material& material = Core::getScene().getMaterials()[handle];
    bool updated = false;
    ImGui::PushItemWidth(-FLT_MIN);

    ui::drawKeyframeButton(handle, "albedo"); ImGui::Text("Albedo:");
    if (ImGui::ColorEdit3("##Mat Albedo", glm::value_ptr(material.albedo)))
        updated = true;

    ImGui::PopItemWidth();
    return updated;
}

bool drawMaterialUI(MaterialHandle handle) {
    Material& material = Core::getScene().getMaterials()[handle];
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
                material.roughness = 0.3f;
                material.metalness = 0.0f;
                material.ior = 1.5f;
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
                material.ior = 1.5f;
                break;
            case MaterialType::Dielectric:
                material.albedo = glm::vec3(1.0f);
                material.roughness = 0.0f;
                material.ior = 1.5f;
                material.density = 0.0f;
                material.transmission = 0.0f;
                break;
            default:
                break;
        }
    }

    switch (material.type) {
        case MaterialType::Principled:   updated |= drawPrincipledUI(handle);   break;
        case MaterialType::Emissive:     updated |= drawEmissiveUI(handle);     break;
        case MaterialType::Lambertian:   updated |= drawLambertianUI(handle);   break;
        case MaterialType::GgxMetal:     updated |= drawGgxMetalUI(handle);     break;
        case MaterialType::GgxGlossy:    updated |= drawGgxGlossyUI(handle);    break;
        case MaterialType::Dielectric:   updated |= drawDielectricUI(handle);   break;
        case MaterialType::Volume:       updated |= drawVolumeUI(handle);       break;
        case MaterialType::Programmable: updated |= drawProgrammableUI(handle); break;
    }

    return updated;
}
