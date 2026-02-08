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

#define ggxRoughness(mat) mat.payload[0]
#define ggxIoR(mat) mat.payload[1]

vec3 schlick(float cosine, vec3 F0) {
    return F0 + (1-F0) * pow((1 - cosine), 5);
}

vec3 schlickApprox(float cosine, float ri) {
    float F0 = (1 - ri) / (1 + ri);
    F0 = F0*F0;
    return schlick(cosine, vec3(F0));
}

vec3 schlickAlbedo(float cosine, vec3 albedo) {
    return schlick(cosine, albedo);
}

struct SampleResult {
    vec3 f;
    vec3 wi;
    float pdf;
    bool isDelta;
};

// ============== Scatter ==============
vec3 cosineScatter(in Material mat, in vec3 normal, in vec3 wo, inout uint seed) {
    vec3 dir = normal + normalize(randomInSphere(seed));
    if (length(dir) < EPS) return normal;
    return normalize(dir);
}

vec3 ggxScatter(in Material mat, in vec3 normal, in vec3 wo, inout uint seed) {
    float s1 = rand(seed);
    float s2 = rand(seed);
    float alpha = ggxRoughness(mat) * ggxRoughness(mat);

    float phi = 2.0 * PI * s1;
    float theta = atan(alpha * sqrt(s2 / sqrt(1 - s2)));

    vec3 local = vec3(
        sin(theta) * cos(phi),
        sin(theta) * sin(phi),
        cos(theta)
    );

    // Build tangent basis
    vec3 t;
    if (abs(normal.z) < 1 - EPS) {
        t = normalize(cross(vec3(0.0, 0.0, 1.0), normal));
    } else {
        t = normalize(cross(vec3(0.0, 1.0, 0.0), normal));
    }
    vec3 b = cross(normal, t);

    vec3 m = normalize(local.x * t + local.y * b + local.z * normal);
    return reflect(-wo, m);
}

// ============== Lambertian ==============
float lambertianPDF(in Material mat, in vec3 normal, in vec3 wo, in vec3 wi) {
    return max(dot(normal, wi), 0.0) / PI;
}

vec3 lambertianF(in Material mat, in vec3 normal, in vec3 wo, in vec3 wi) {
    return mat.albedo / PI;
}

void sampleLambertianBSDF(in Material mat, in Hit hit, in vec3 wo, out SampleResult result, inout uint seed) {
    result.wi = cosineScatter(mat, hit.normal, wo, seed);

    result.f = lambertianF(mat, hit.normal, wo, result.wi);
    result.pdf = lambertianPDF(mat, hit.normal, wo, result.wi);
    result.isDelta = false;
}

// ============== GGX ==============

#define Xp(x) ((x) > 0.0 ? 1.0 : 0.0)
float D_ggx(in float alpha, in vec3 normal, in vec3 m) {
    float NoM = max(dot(normal, m), 0.0);
    if (NoM <= 0.0) return 0.0;

    float a_sq = alpha * alpha;
    float denom = NoM*NoM * (a_sq - 1.0) + 1.0;
    return a_sq / (PI * denom * denom);
}

float G1_ggx(in float alpha, in vec3 normal, in vec3 v) {
    float NoV = dot(normal, v);
    if (NoV <= 0.0) return 0.0;

    float a_sq = alpha * alpha;
    float NoV_sq = NoV * NoV;
    float tan_sq = (1.0 - NoV_sq) / max(NoV_sq, EPS);
    return 2.0 / (1.0 + sqrt(1.0 + a_sq * tan_sq));
}

float ggxPDF(in Material mat, in vec3 normal, in vec3 wo, in vec3 wi) {
    vec3 m = normalize(wi + wo);
    float NoM = max(dot(normal, m), 0.0);
    float VoM = max(dot(wo, m), 0.0);
    if (NoM <= 0.0 || VoM <= 0.0) return 0.0;

    float alpha = ggxRoughness(mat) * ggxRoughness(mat);
    float D = D_ggx(alpha, normal, m);

    return (D * NoM) / (4.0 * VoM);
}

vec3 ggxF(in Material mat, in vec3 normal, in vec3 wo, in vec3 wi) {
    float alpha = ggxRoughness(mat) * ggxRoughness(mat);
    vec3 m = normalize(wo + wi);
    
    float D = D_ggx(alpha, normal, m);
    float G = G1_ggx(alpha, normal, wo) * G1_ggx(alpha, normal, wi);
    // vec3 F = schlickApprox(dot(wo, m), ggxIoR(mat));
    vec3 F = schlickAlbedo(dot(wo, m), mat.albedo);
    
    float cosWo = abs(dot(normal, wo));
    float cosWi = abs(dot(normal, wi));
    vec3 f = F * D * G / (4 * cosWo * cosWi + EPS);
    return f;
}

void sampleMirrorBSDF(in Material mat, in Hit hit, in vec3 wo, out SampleResult result) {
    result.wi = reflect(-wo, hit.normal);
    
    float VoN = max(dot(wo, hit.normal), 0.0);
    // result.f = schlickApprox(dot(wo, hit.normal), ggxIoR(mat));
    result.f = schlickAlbedo(VoN, mat.albedo) / VoN;
    result.pdf = 1.0;
    result.isDelta = true;
}

void sampleGgxBSDF(in Material mat, in Hit hit, in vec3 wo, out SampleResult result, inout uint seed) {
    if (ggxRoughness(mat) < 0.05) {
        sampleMirrorBSDF(mat, hit, wo, result);
    } else {
        result.wi = ggxScatter(mat, hit.normal, wo, seed);

        result.f = ggxF(mat, hit.normal, wo, result.wi);
        result.pdf = ggxPDF(mat, hit.normal, wo, result.wi);
        result.isDelta = ggxRoughness(mat) < EPS;
    }
}

// ============== General functions ==============
float samplePDF(in Material mat, in vec3 normal, in vec3 wo, in vec3 wi) {
    switch (mat.type) {
        case mat_Lambertian: return lambertianPDF(mat, normal, wo, wi);
        case mat_Glossy: return ggxPDF(mat, normal, wo, wi);
        default : return lambertianPDF(mat, normal, wo, wi);
    }
}

vec3 sampleF(in Material mat, in vec3 normal, in vec3 wo, in vec3 wi) {
    switch (mat.type) {
        case mat_Lambertian: return lambertianF(mat, normal, wo, wi);
        case mat_Glossy: return ggxF(mat, normal, wo, wi);
        default : return lambertianF(DEFAULT_MATERIAL, normal, wo, wi);
    }
}

void sampleBSDF(in Material mat, in Hit hit, in vec3 wo, out SampleResult result, inout uint seed) {
    switch (mat.type) {
        case mat_Lambertian: sampleLambertianBSDF(mat, hit, wo, result, seed); break;
        case mat_Glossy: sampleGgxBSDF(mat, hit, wo, result, seed); break;
        default : sampleLambertianBSDF(DEFAULT_MATERIAL, hit, wo, result, seed); break;
    }
}

#endif
