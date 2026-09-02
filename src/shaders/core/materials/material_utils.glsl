#ifndef MATERIAL_UTILS_GLSL
#define MATERIAL_UTILS_GLSL

#include "../utils.glsl"

float luma(vec3 c) {
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}


struct Material {
    int   type;
    float payload[12];
};

#define albedo(m)                    vec3((m).payload[0], (m).payload[1], (m).payload[2])

#define emissionStrength(m) (m).payload[3]

#define metalRoughness(m)           (m).payload[3]

#define glossyRoughness(m)          (m).payload[3]
#define glossyIor(m)                (m).payload[4]

#define dielectricRoughness(m)      (m).payload[3]
#define dielectricIor(m)            (m).payload[4]
#define dielectricTransmission(m)   (m).payload[5]
#define dielectricDensity(m)        (m).payload[6]
#define dielectricAnisotropic(m)    (m).payload[7]

#define volumeDensity(m)            (m).payload[3]
#define volumeAnisotropic(m)        (m).payload[4]

#define principledRoughness(m)      (m).payload[3]
#define principledMetalness(m)      (m).payload[4]
#define principledIor(m)            (m).payload[5]
#define principledTransmission(m)   (m).payload[6]
#define principledDensity(m)        (m).payload[7]
#define principledAnisotropic(m)    (m).payload[8]
#define principledAlpha(m)          (m).payload[9]

void setAlbedo(inout Material m, vec3 v) {
    m.payload[0] = v.r; m.payload[1] = v.g; m.payload[2] = v.b;
}

Material Diffuse(vec3 albedo) {
    Material m;
    m.type = mat_Diffuse;
    m.payload[0] = albedo.r; m.payload[1] = albedo.g; m.payload[2] = albedo.b;
    return m;
}

Material Glossy(vec3 albedo, float roughness, float ior) {
    Material m;
    m.type = mat_Glossy;
    m.payload[0] = albedo.r; m.payload[1] = albedo.g; m.payload[2] = albedo.b;
    m.payload[3] = roughness;
    m.payload[4] = ior;
    return m;
}

Material Metal(vec3 albedo, float roughness) {
    Material m;
    m.type = mat_Metal;
    m.payload[0] = albedo.r; m.payload[1] = albedo.g; m.payload[2] = albedo.b;
    m.payload[3] = roughness;
    return m;
}

// ============== BSDF ==============
struct BSDFMediumInfo {
    bool isDielectric;
    bool isVolume;
    vec3 absorption;
    float density;
    float scatterAlbedo;
    float anisotropic;   // `g` in the Henyey–Greenstein phase function (https://en.wikipedia.org/wiki/Henyey–Greenstein_phase_function)
};

struct BSDFSample {
    vec3 weight;
    vec3 wi;
    float pdf;
    bool isDelta;
    BSDFMediumInfo medium;
};

struct BSDFEval {
    vec3 f;
    float pdf;
};

#define DEFAULT_MATERIAL materialBuffer.materials[0]

// ============== REFRACTION/REFLECTION ==============
#define SCHLICK_APPROX(cosine, F0) F0 + (1-F0) * pow((1 - cosine), 5)

vec3 schlickIoR(float cosine, float ri) {
    float F0 = (1 - ri) / (1 + ri);
    F0 = F0*F0;
    return SCHLICK_APPROX(cosine, vec3(F0));
}

float fresnelDielectric(float cosI, float etaI, float etaT) {
    float sinT2 = (etaI / etaT) * (etaI / etaT) * (1.0 - cosI * cosI);
    if (sinT2 >= 1.0) return 1.0;
    float cosT = sqrt(1.0 - sinT2);
    float Rs = (etaI * cosI - etaT * cosT) / (etaI * cosI + etaT * cosT);
    float Rp = (etaT * cosI - etaI * cosT) / (etaT * cosI + etaI * cosT);
    return (Rs * Rs + Rp * Rp) * 0.5;
}

vec3 schlickAlbedo(float cosine, vec3 albedo) {
    return SCHLICK_APPROX(cosine, albedo);
}

bool isTransmissive(in Material mat) {
    if (mat.type == mat_Dielectric) return true;
    if (mat.type == mat_Principled) return (1.0 - principledMetalness(mat)) * principledTransmission(mat) > EPS;
    return false;
}

// ============== MIRROR ==============
BSDFEval evalMirrorBSDF(in vec3 albedo, in Hit hit, in vec3 wo, in vec3 wi) {
    float VoN = max(dot(wo, hit.normal), 0.0);
    return BSDFEval(
        albedo * schlickIoR(VoN, 0.0) / VoN,
        0.0
    );
}

BSDFSample sampleMirrorBSDF(in vec3 albedo, in Hit hit, in vec3 wo) {
    vec3 wi = reflect(-wo, hit.normal);
    float cosB = abs(dot(hit.normal, wi));
    BSDFEval eval = evalMirrorBSDF(albedo, hit, wo, wi);
    
    BSDFSample bsdf;
    bsdf.wi = wi;
    bsdf.weight = eval.f * cosB;
    bsdf.pdf = eval.pdf;
    bsdf.isDelta = true;
    bsdf.medium.isDielectric = false;
    return bsdf;
}

#endif // MATERIAL_UTILS_GLSL
