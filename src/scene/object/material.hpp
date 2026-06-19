#pragma once

#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

typedef int MaterialHandle;

enum MaterialType {
    Principled = 0,
    Emissive,
    Lambertian,
    GgxMetal,
    GgxGlossy,
    Dielectric,
    Programmable,
};

struct GpuMaterial {
    MaterialType type;
    alignas(16) glm::vec3 albedo;
    float roughness;
    float metalness;
    float ior;
    float transmission;
    float emissionStrength;
};

struct Material {
    std::string name;
    MaterialType type;
    glm::vec3 albedo;
    float roughness        = 0.0f;
    float metalness        = 0.0f;
    float ior              = 0.0f;
    float transmission     = 0.0f;
    float emissionStrength = 0.0f;
};

#define DEFAULT_MATERIAL Material{ .name = "Default", .type = MaterialType::Lambertian, .albedo = glm::vec3(1.0f, 0.0f, 1.0f)*0.6f }

bool drawPrincipledUI(Material& mat);
bool drawEmissiveUI(Material& mat);
bool drawLambertianUI(Material& mat);
bool drawGgxMetalUI(Material& mat);
bool drawGgxGlossyUI(Material& mat);
bool drawDielectricUI(Material& mat);
bool drawProgrammableUI(Material& mat);

bool drawMaterialUI(Material& mat);
