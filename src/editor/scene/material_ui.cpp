#include "material_ui.hpp"

#include <glm/gtc/type_ptr.hpp>

#include "imgui/imgui.h"

#include "core/core.hpp"
#include "core/scene/scene.hpp"
#include "editor/ui_utils.hpp"

static bool drawPrincipledUI(MaterialHandle handle) {
    Material& material = Core::getScene().getMaterials()[handle];
    bool updated = false;
    ImGui::PushItemWidth(-FLT_MIN);

    ui::drawKeyframeButton(handle, "albedo"); ImGui::Text("Albedo:");
    { glm::vec3 v = material.get<glm::vec3>("albedo");
      if (ImGui::ColorEdit3("##Mat Albedo", glm::value_ptr(v)))
          { material.set<glm::vec3>("albedo", v); updated = true; } }

    ui::drawKeyframeButton(handle, "roughness"); ImGui::Text("Roughness:");
    { float v = material.get<float>("roughness");
      if (ImGui::SliderFloat("##Mat Roughness", &v, 0.0f, 1.0f))
          { material.set<float>("roughness", v); updated = true; } }

    ui::drawKeyframeButton(handle, "metalness"); ImGui::Text("Metalness:");
    { float v = material.get<float>("metalness");
      if (ImGui::SliderFloat("##Mat Metalness", &v, 0.0f, 1.0f))
          { material.set<float>("metalness", v); updated = true; } }

    ui::drawKeyframeButton(handle, "ior"); ImGui::Text("IoR:");
    { float v = material.get<float>("ior");
      if (ImGui::DragFloat("##Mat IoR", &v, 0.01f, 1.0f, FLT_MAX))
          { material.set<float>("ior", v); updated = true; } }

    ui::drawKeyframeButton(handle, "transmission"); ImGui::Text("Transmission:");
    { float v = material.get<float>("transmission");
      if (ImGui::SliderFloat("##Mat Transmission", &v, 0.0f, 1.0f))
          { material.set<float>("transmission", v); updated = true; } }

    if (material.get<float>("transmission") > 0.0f) {
        ui::drawKeyframeButton(handle, "density"); ImGui::Text("Density:");
        { float v = material.get<float>("density");
          if (ImGui::DragFloat("##Mat Density", &v, 0.01f, 0.0f, FLT_MAX))
              { material.set<float>("density", v); updated = true; } }
    }

    ImGui::PopItemWidth();
    return updated;
}

static bool drawEmissiveUI(MaterialHandle handle) {
    Material& material = Core::getScene().getMaterials()[handle];
    bool updated = false;
    ImGui::PushItemWidth(-FLT_MIN);

    ui::drawKeyframeButton(handle, "albedo"); ImGui::Text("Albedo:");
    { glm::vec3 v = material.get<glm::vec3>("albedo");
      if (ImGui::ColorEdit3("##Mat Albedo", glm::value_ptr(v)))
          { material.set<glm::vec3>("albedo", v); updated = true; } }

    ui::drawKeyframeButton(handle, "emissionStrength"); ImGui::Text("Intensity:");
    { float v = material.get<float>("emissionStrength");
      if (ImGui::DragFloat("##Mat Intensity", &v, 0.1f, 0.0f, FLT_MAX))
          { material.set<float>("emissionStrength", v); updated = true; } }

    ImGui::PopItemWidth();
    return updated;
}

static bool drawLambertianUI(MaterialHandle handle) {
    Material& material = Core::getScene().getMaterials()[handle];
    bool updated = false;
    ImGui::PushItemWidth(-FLT_MIN);

    ui::drawKeyframeButton(handle, "albedo"); ImGui::Text("Albedo:");
    { glm::vec3 v = material.get<glm::vec3>("albedo");
      if (ImGui::ColorEdit3("##Mat Albedo", glm::value_ptr(v)))
          { material.set<glm::vec3>("albedo", v); updated = true; } }

    ImGui::PopItemWidth();
    return updated;
}

static bool drawGgxMetalUI(MaterialHandle handle) {
    Material& material = Core::getScene().getMaterials()[handle];
    bool updated = false;
    ImGui::PushItemWidth(-FLT_MIN);

    ui::drawKeyframeButton(handle, "albedo"); ImGui::Text("Albedo:");
    { glm::vec3 v = material.get<glm::vec3>("albedo");
      if (ImGui::ColorEdit3("##Mat Albedo", glm::value_ptr(v)))
          { material.set<glm::vec3>("albedo", v); updated = true; } }

    ui::drawKeyframeButton(handle, "roughness"); ImGui::Text("Roughness:");
    { float v = material.get<float>("roughness");
      if (ImGui::SliderFloat("##Mat Roughness", &v, 0.0f, 1.0f))
          { material.set<float>("roughness", v); updated = true; } }

    ImGui::PopItemWidth();
    return updated;
}

static bool drawGgxGlossyUI(MaterialHandle handle) {
    Material& material = Core::getScene().getMaterials()[handle];
    bool updated = false;
    ImGui::PushItemWidth(-FLT_MIN);

    ui::drawKeyframeButton(handle, "albedo"); ImGui::Text("Albedo:");
    { glm::vec3 v = material.get<glm::vec3>("albedo");
      if (ImGui::ColorEdit3("##Mat Albedo", glm::value_ptr(v)))
          { material.set<glm::vec3>("albedo", v); updated = true; } }

    ui::drawKeyframeButton(handle, "roughness"); ImGui::Text("Roughness:");
    { float v = material.get<float>("roughness");
      if (ImGui::SliderFloat("##Mat Roughness", &v, 0.0f, 1.0f))
          { material.set<float>("roughness", v); updated = true; } }

    ui::drawKeyframeButton(handle, "ior"); ImGui::Text("IoR:");
    { float v = material.get<float>("ior");
      if (ImGui::DragFloat("##Mat IoR", &v, 0.01f, 1.0f, FLT_MAX))
          { material.set<float>("ior", v); updated = true; } }

    ImGui::PopItemWidth();
    return updated;
}

static bool drawDielectricUI(MaterialHandle handle) {
    Material& material = Core::getScene().getMaterials()[handle];
    bool updated = false;
    ImGui::PushItemWidth(-FLT_MIN);

    ui::drawKeyframeButton(handle, "albedo"); ImGui::Text("Albedo:");
    { glm::vec3 v = material.get<glm::vec3>("albedo");
      if (ImGui::ColorEdit3("##Mat Albedo", glm::value_ptr(v)))
          { material.set<glm::vec3>("albedo", v); updated = true; } }

    ui::drawKeyframeButton(handle, "ior"); ImGui::Text("IoR:");
    { float v = material.get<float>("ior");
      if (ImGui::DragFloat("##Mat IoR", &v, 0.01f, 1.0f, FLT_MAX))
          { material.set<float>("ior", v); updated = true; } }

    ui::drawKeyframeButton(handle, "roughness"); ImGui::Text("Roughness:");
    { float v = material.get<float>("roughness");
      if (ImGui::SliderFloat("##Mat Roughness", &v, 0.0f, 1.0f))
          { material.set<float>("roughness", v); updated = true; } }

    ui::drawKeyframeButton(handle, "density"); ImGui::Text("Density:");
    { float v = material.get<float>("density");
      if (ImGui::DragFloat("##Mat Density", &v, 0.01f, 0.0f, FLT_MAX))
          { material.set<float>("density", v); updated = true; } }

    if (material.get<float>("density") > 0.0f) {
        ui::drawKeyframeButton(handle, "transmission"); ImGui::Text("Scatter albedo:");
        { float v = material.get<float>("transmission");
          if (ImGui::SliderFloat("##Mat Transmission", &v, 0.0f, 1.0f))
              { material.set<float>("transmission", v); updated = true; } }

        if (material.get<float>("transmission") > 0.0f) {
            ui::drawKeyframeButton(handle, "anisotropic"); ImGui::Text("Anisotropic:");
            { float v = material.get<float>("anisotropic");
              if (ImGui::DragFloat("##Mat Anisotropic", &v, 0.01f, -1.0f, 1.0f))
                  { material.set<float>("anisotropic", v); updated = true; } }
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
    { glm::vec3 v = material.get<glm::vec3>("albedo");
      if (ImGui::ColorEdit3("##Mat Albedo", glm::value_ptr(v)))
          { material.set<glm::vec3>("albedo", v); updated = true; } }

    ui::drawKeyframeButton(handle, "density"); ImGui::Text("Density:");
    { float v = material.get<float>("density");
      if (ImGui::DragFloat("##Mat Density", &v, 0.01f, 0.0f, FLT_MAX))
          { material.set<float>("density", v); updated = true; } }

    ui::drawKeyframeButton(handle, "anisotropic"); ImGui::Text("Anisotropic:");
    { float v = material.get<float>("anisotropic");
      if (ImGui::SliderFloat("##Mat Anisotropic", &v, -1.0f, 1.0))
          { material.set<float>("anisotropic", v); updated = true; } }

    ImGui::PopItemWidth();
    return updated;
}

static bool drawProgrammableUI(MaterialHandle handle) {
    Material& material = Core::getScene().getMaterials()[handle];
    bool updated = false;
    ImGui::PushItemWidth(-FLT_MIN);

    ui::drawKeyframeButton(handle, "albedo"); ImGui::Text("Albedo:");
    { glm::vec3 v = material.get<glm::vec3>("albedo");
      if (ImGui::ColorEdit3("##Mat Albedo", glm::value_ptr(v)))
          { material.set<glm::vec3>("albedo", v); updated = true; } }

    ImGui::PopItemWidth();
    return updated;
}

bool drawMaterialUI(MaterialHandle handle) {
    Material& material = Core::getScene().getMaterials()[handle];
    bool updated = false;
    MaterialType prevType = material.getType();

    ImGui::PushItemWidth(-FLT_MIN);

    ImGui::Text("Name:");
    char nameBuf[128] = {};
    material.getName().copy(nameBuf, sizeof(nameBuf) - 1);
    if (ImGui::InputText("##Name", nameBuf, sizeof(nameBuf)))
        material.setName(nameBuf);

    const char* types[] = { "Principled", "Emissive", "Lambertian", "GGX Metal", "GGX Glossy", "Dielectric", "Volume", "Programmable" };
    ImGui::Text("Type:");
    int matTypeIdx = static_cast<int>(material.getType());
    if (ImGui::Combo("##Mat Type", &matTypeIdx, types, IM_ARRAYSIZE(types))) {
        material.setType(static_cast<MaterialType>(matTypeIdx));
        updated = true;
    }

    ImGui::PopItemWidth();

    if (material.getType() != prevType) {
        switch (material.getType()) {
            case MaterialType::Principled:
                material.set<float>("roughness", 0.3f);
                material.set<float>("metalness", 0.0f);
                material.set<float>("ior", 1.5f);
                material.set<float>("transmission", 0.0f);
                break;
            case MaterialType::Emissive:
                material.set<float>("emissionStrength", 1.0f);
                break;
            case MaterialType::GgxMetal:
                material.set<float>("roughness", 0.2f);
                break;
            case MaterialType::GgxGlossy:
                material.set<float>("roughness", 0.2f);
                material.set<float>("ior", 1.5f);
                break;
            case MaterialType::Dielectric:
                material.set<glm::vec3>("albedo", glm::vec3(1.0f));
                material.set<float>("roughness", 0.0f);
                material.set<float>("ior", 1.5f);
                material.set<float>("density", 0.0f);
                material.set<float>("transmission", 0.0f);
                break;
            default:
                break;
        }
    }

    switch (material.getType()) {
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
