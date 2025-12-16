#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui/imgui.h"

enum MaterialType {
    Lambertian = 0,
    Metal,
    Dielectric,
    Emissive,
    Glossy,
    Checkerboard,
};

struct Material {
    MaterialType type;
    alignas(16) glm::vec3 albedo;
    float payload[2];
    // float fuzz;
    // float refraction_index;
    // float intensity;
};

#define metalFuzz(mat) mat.payload[0]
#define dielectricIoR(mat) mat.payload[0]
#define emissiveIntensity(mat) mat.payload[0]
#define glossyIoR(mat) mat.payload[0]
#define glossyFuzz(mat) mat.payload[1]
#define checkerboardScale(mat) mat.payload[0]

typedef int MaterialHandle;

bool drawLambertianUI(Material &mat);
bool drawMetalUI(Material &mat);
bool drawDielectricUI(Material &mat);
bool drawEmissiveUI(Material &mat);
bool drawGlossyUI(Material &mat);
bool drawCheckerboardUI(Material &mat);

bool drawMaterialUI(Material &mat);
