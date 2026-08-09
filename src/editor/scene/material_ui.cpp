#include "material_ui.hpp"

#include <string>

#include "imgui/imgui.h"

#include "core/core.hpp"
#include "core/scene/scene.hpp"
#include "editor/field_ui.hpp"
#include "editor/ui_utils.hpp"

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
                material.set<float>("density", 1.0f);
                material.set<float>("alpha", 1.0f);
                break;
            case MaterialType::Emissive:
                material.set<float>("emission_strength", 1.0f);
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

    ImGui::PushItemWidth(-FLT_MIN);

    auto field = [&](const std::string& id) -> bool {
        ui::drawKeyframeButton(handle, id);
        return ui::drawField(material.getField(id), "##Mat_" + id);
    };

    switch (material.getType()) {
        case MaterialType::Principled:
            updated |= field("albedo");
            updated |= field("roughness");
            updated |= field("metalness");
            updated |= field("ior");
            updated |= field("transmission");
            if (material.get<float>("transmission") > 0.0f) updated |= field("density");
            updated |= field("alpha");
            break;
        case MaterialType::Emissive:
            updated |= field("albedo");
            updated |= field("emission_strength");
            break;
        case MaterialType::Lambertian:
            updated |= field("albedo");
            break;
        case MaterialType::GgxMetal:
            updated |= field("albedo");
            updated |= field("roughness");
            break;
        case MaterialType::GgxGlossy:
            updated |= field("albedo");
            updated |= field("roughness");
            updated |= field("ior");
            break;
        case MaterialType::Dielectric:
            updated |= field("albedo");
            updated |= field("ior");
            updated |= field("roughness");
            updated |= field("density");
            if (material.get<float>("density") > 0.0f) {
                updated |= field("transmission");
                if (material.get<float>("transmission") > 0.0f)
                    updated |= field("anisotropic");
            }
            break;
        case MaterialType::Volume:
            updated |= field("albedo");
            updated |= field("density");
            updated |= field("anisotropic");
            break;
        case MaterialType::Programmable:
            updated |= field("albedo");
            break;
    }

    ImGui::PopItemWidth();
    return updated;
}
