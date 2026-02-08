#pragma once

#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>


#include "imgui/imgui.h"

typedef int MaterialHandle;

enum MaterialType {
    Lambertian = 0,
    Metal,
    Dielectric,
    Emissive,
    Glossy,
    Checkerboard,
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

#define metalFuzz(mat) mat.payload[0]
#define dielectricIoR(mat) mat.payload[0]
#define dielectricFuzz(mat) mat.payload[1]
#define emissiveIntensity(mat) mat.payload[0]
#define glossyIoR(mat) mat.payload[0]
#define glossyFuzz(mat) mat.payload[1]
#define checkerboardScale(mat) mat.payload[0]


bool drawLambertianUI(Material &mat);
bool drawMetalUI(Material &mat);
bool drawDielectricUI(Material &mat);
bool drawEmissiveUI(Material &mat);
bool drawGlossyUI(Material &mat);
bool drawCheckerboardUI(Material &mat);

bool drawMaterialUI(Material &mat);
