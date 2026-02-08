#pragma once

#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>


#include "imgui/imgui.h"

typedef int MaterialHandle;

enum MaterialType {
    Lambertian = 0,
    Emissive,
    GgxMetal,
    GgxPlastic,
    Programmable,
};

struct GpuMaterial {
    MaterialType type;
    alignas(16) glm::vec3 albedo;
    float payload[2];
};

struct Material {
    std::string name;
    MaterialType type;
    glm::vec3 albedo;
    float payload[2];
};

#define DEFAULT_MATERIAL Material{ .name = "Default", .type = MaterialType::Lambertian, .albedo = glm::vec3(1.0f, 0.0f, 1.0f)*0.6f }

#define emissiveIntensity(mat) mat.payload[0]
#define ggxMetalFuzz(mat) mat.payload[0]
#define ggxPlasticRoughness(mat) mat.payload[0]
#define ggxPlasticIoR(mat) mat.payload[1]

bool drawLambertianUI(Material &mat);
bool drawEmissiveUI(Material &mat);
bool drawGgxMetalUI(Material &mat);
bool drawGgxPlasticUI(Material &mat);
bool drawProgrammableUI(Material &mat);

bool drawMaterialUI(Material &mat);
