#ifndef MATERIAL_UTILS_GLSL
#define MATERIAL_UTILS_GLSL

#include "../utils.glsl"

struct SampleResult {
    vec3 f;
    vec3 wi;
    float pdf;
    bool isDelta;
};

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

#define DEFAULT_MATERIAL                         SET_MATERIAL_0(mat_Lambertian, vec3(1,0,1)*0.7)
#define LAMBERTIAN_MATERIAL(albedo)              SET_MATERIAL_0(mat_Lambertian, albedo)
#define GGX_METAL_MATERIAL(albedo, rough)        SET_MATERIAL_1(mat_GgxMetal, albedo, rough)
#define GGX_GLOSSY_MATERIAL(albedo, rough, ior) SET_MATERIAL_2(mat_GgxGlossy, albedo, rough, ior)

#define emissiveIntensity(mat)  mat.payload[0]
#define ggxMetalRoughness(mat)  mat.payload[0]
#define ggxGlossyRoughness(mat) mat.payload[0]
#define ggxGlossyIoR(mat)       mat.payload[1]
#define dielectricIoR(mat)      mat.payload[0]
#define dielectricRoughness(mat) mat.payload[1]

#define SCHLICK_APPROX(cosine, F0) F0 + (1-F0) * pow((1 - cosine), 5)

vec3 schlickIoR(float cosine, float ri) {
    float F0 = (1 - ri) / (1 + ri);
    F0 = F0*F0;
    return SCHLICK_APPROX(cosine, vec3(F0));
}

vec3 schlickAlbedo(float cosine, vec3 albedo) {
    return SCHLICK_APPROX(cosine, albedo);
}

void sampleMirrorBSDF(in vec3 albedo, in Hit hit, in vec3 wo, out SampleResult result) {
    result.wi = reflect(-wo, hit.normal);

    float VoN = max(dot(wo, hit.normal), 0.0);
    result.f = albedo * schlickIoR(VoN, 0.0) / VoN;
    result.pdf = 1.0;
    result.isDelta = true;
}

#endif // MATERIAL_UTILS_GLSL
