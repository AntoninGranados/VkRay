#ifndef MATERIALS_GLSL
#define MATERIALS_GLSL

#include "utils.glsl"
#include "random.glsl"

Material makeMaterial(Enum type, vec3 albedo, float f0, float f1) {
    Material m;
    m.type = type;
    m.albedo = albedo;
    m.payload[0] = f0;
    m.payload[1] = f1;
    return m;
}

#define SET_MATERIAL_0(type, albedo)         makeMaterial(type, albedo, 0.0, 0.0)
#define SET_MATERIAL_1(type, albedo, f0)     makeMaterial(type, albedo, f0, 0.0)
#define SET_MATERIAL_2(type, albedo, f0, f1) makeMaterial(type, albedo, f0, f1)

#define DEFAULT_MATERIAL                     SET_MATERIAL_0(mat_Lambertian, vec3(1,0,1)*0.7)
#define LAMBERTIAN_MATERIAL(albedo)          SET_MATERIAL_0(mat_Lambertian, albedo)
#define METAL_MATERIAL(albedo, fuzz)         SET_MATERIAL_1(mat_Metal, albedo, fuzz)
#define DIELECTRIX_MATERIAL(albedo, ior)     SET_MATERIAL_1(mat_Dielectric, albedo, ior)
#define EMISSIVE_MATERIAL(albedo, intensity) SET_MATERIAL_1(mat_Emissive, albedo, intensity)
#define GLOSSY_MATERIAL(albedo, ior, fuzz)   SET_MATERIAL_2(mat_Glossy, albedo, ior, fuzz)
#define CHECKERBOARD_MATERIAL                SET_MATERIAL_1(mat_Checkerboard, vec3(0), 2)

#define metalFuzz(mat) mat.payload[0]
#define dielectricIoR(mat) mat.payload[0]
#define dielectricFuzz(mat) mat.payload[1]
#define emissiveIntensity(mat) mat.payload[0]
#define glossyIoR(mat) mat.payload[0]
#define glossyFuzz(mat) mat.payload[1]
#define checkerboardScale(mat) mat.payload[0]

float schlick_approx(float cosine, float ri) {
    float r0 = (1 - ri) / (1 + ri);
    r0 = r0*r0;
    return r0 + (1-r0) * pow((1 - cosine),5);
}

struct ScatterResult {
    vec3 attenuation;
    Ray scattered;
    bool isScattered;
    bool isDiffuse;
};

// ================== SCATTERING FUNCIONS ==================
void scatterLambertian(in Material mat, in Ray ray, in Hit hit, out ScatterResult result, inout uint seed) {
    vec3 dir = hit.normal + randomInSphere(seed);
    if (length(dir) < EPS) dir = hit.normal;
    dir = normalize(dir);

    result.scattered = Ray(hit.p + hit.normal * EPS, dir);
    result.attenuation = mat.albedo / PI;
    result.isScattered = true;
    result.isDiffuse = true;
}

void scatter(in Material mat, in Ray ray, in Hit hit, out ScatterResult result, inout uint seed) {
    scatterLambertian(LAMBERTIAN_MATERIAL(mat.albedo), ray, hit, result, seed);
}

#endif
