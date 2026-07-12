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
    Volume,
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
    float density;
    float anisotropic;
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
    float density          = 1.0f;
    float anisotropic      = 0.0f;
};

inline const Material DEFAULT_MATERIAL{ .name = "Default", .type = MaterialType::Lambertian, .albedo = glm::vec3(1.0f, 0.0f, 1.0f)*0.6f };
