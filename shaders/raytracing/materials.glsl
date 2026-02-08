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
#define METAL_MATERIAL(albedo, roughness)    SET_MATERIAL_1(mat_Metal, albedo, roughness)

#define emissiveIntensity(mat) mat.payload[0]
#define ggxMetalRoughness(mat) mat.payload[0]
#define ggxPlasticRoughness(mat) mat.payload[0]
#define ggxPlasticIoR(mat) mat.payload[1]

vec3 schlickApprox(float cosine, vec3 F0) {
    return F0 + (1-F0) * pow((1 - cosine), 5);
}

vec3 schlickIoR(float cosine, float ri) {
    float F0 = (1 - ri) / (1 + ri);
    F0 = F0*F0;
    return schlickApprox(cosine, vec3(F0));
}

vec3 schlickAlbedo(float cosine, vec3 albedo) {
    return schlickApprox(cosine, albedo);
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

vec3 ggxScatter(in Material mat, in vec3 normal, in vec3 wo, in float alpha, inout uint seed) {
    float s1 = rand(seed);
    float s2 = rand(seed);

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

    float alpha = ggxMetalRoughness(mat) * ggxMetalRoughness(mat);
    float D = D_ggx(alpha, normal, m);

    return (D * NoM) / (4.0 * VoM);
}

float partialGgxF(in Material mat, in vec3 normal, in vec3 wo, in vec3 wi, in float alpha) {
    // GGX f without the F (Fresnel) component
    vec3 m = normalize(wo + wi);
    
    float D = D_ggx(alpha, normal, m);
    float G = G1_ggx(alpha, normal, wo) * G1_ggx(alpha, normal, wi);
    
    float cosWo = abs(dot(normal, wo));
    float cosWi = abs(dot(normal, wi));
    return D * G / (4 * cosWo * cosWi + EPS);
}

void sampleMirrorBSDF(in Material mat, in Hit hit, in vec3 wo, out SampleResult result) {
    result.wi = reflect(-wo, hit.normal);

    float VoN = max(dot(wo, hit.normal), 0.0);
    result.f = schlickAlbedo(VoN, mat.albedo) / VoN;
    result.pdf = 1.0;
    result.isDelta = true;
}

// ============== GGX Metal ==============

vec3 ggxMetalF(in Material mat, in vec3 normal, in vec3 wo, in vec3 wi) {
    float alpha = ggxMetalRoughness(mat) * ggxMetalRoughness(mat);
    vec3 m = normalize(wo + wi);
    vec3 F = schlickAlbedo(dot(wo, m), mat.albedo);
    return F * partialGgxF(mat, normal, wo, wi, alpha);
}

void sampleGgxMetalBSDF(in Material mat, in Hit hit, in vec3 wo, out SampleResult result, inout uint seed) {
    if (ggxMetalRoughness(mat) < 0.05) {
        sampleMirrorBSDF(mat, hit, wo, result);
    } else {
        float alpha = ggxMetalRoughness(mat) * ggxMetalRoughness(mat);
        result.wi = ggxScatter(mat, hit.normal, wo, alpha, seed);

        result.f = ggxMetalF(mat, hit.normal, wo, result.wi);
        result.pdf = ggxPDF(mat, hit.normal, wo, result.wi);
        result.isDelta = ggxMetalRoughness(mat) < EPS;
    }
}

// ============== GGX Plastic ==============
vec3 ggxPlasticF(in Material mat, in vec3 normal, in vec3 wo, in vec3 wi) {
    // specular
    float alpha = ggxMetalRoughness(mat) * ggxMetalRoughness(mat);
    vec3 m = normalize(wo + wi);
    vec3 F = schlickAlbedo(dot(wo, m), mat.albedo);
    vec3 fs = F * partialGgxF(mat, normal, wo, wi, alpha);

    // diffuse
    float NoV = max(dot(normal, wo), 0.0);
    vec3  Fv  = schlickIoR(NoV, ggxPlasticIoR(mat));
    vec3 fd = (mat.albedo / PI) * (vec3(1.0) - Fv);

    return fs + fd;
}

float ggxPlasticPDF(in Material mat, in vec3 normal, in vec3 wo, in vec3 wi) {
    float NoV = max(dot(normal, wo), 0.0);
    vec3  Fv  = schlickIoR(NoV, ggxPlasticIoR(mat));
    float ps  = clamp(luma(Fv), 0.05, 0.95);
    float pd  = 1.0 - ps;

    float pdfS = ggxPDF(mat, normal, wo, wi);
    float pdfD = lambertianPDF(mat, normal, wo, wi);

    return ps * pdfS + pd * pdfD;
}

void sampleGgxPlasticBSDF(in Material mat, in Hit hit, in vec3 wo, out SampleResult result, inout uint seed) {
    float rough = ggxPlasticRoughness(mat);
    bool specIsDelta = (rough < 0.05);

    float NoV = max(dot(hit.normal, wo), 0.0);
    vec3  Fv  = schlickIoR(NoV, ggxPlasticIoR(mat));
    float pSpec = clamp(luma(Fv), 0.05, 0.95);
    float pDiff = 1.0 - pSpec;

    float xi = rand(seed);

    if (xi < pSpec) {
        if (specIsDelta) {
            sampleMirrorBSDF(mat, hit, wo, result);
            return;
        } else {
            float alpha = ggxPlasticRoughness(mat) * ggxPlasticRoughness(mat);
            result.wi = ggxScatter(mat, hit.normal, wo, alpha, seed);
            result.isDelta = false;
        }
    } else {
        result.wi = cosineScatter(mat, hit.normal, wo, seed);
        result.isDelta = false;
    }

    result.f   = ggxPlasticF(mat, hit.normal, wo, result.wi);
    result.pdf = ggxPlasticPDF(mat, hit.normal, wo, result.wi);
}

// ============== General functions ==============
float samplePDF(in Material mat, in vec3 normal, in vec3 wo, in vec3 wi) {
    switch (mat.type) {
        case mat_Lambertian:    return lambertianPDF(mat, normal, wo, wi);
        case mat_GgxMetal:      return ggxPDF(mat, normal, wo, wi);
        case mat_GgxPlastic:    return ggxPlasticPDF(mat, normal, wo, wi);
        default :               return lambertianPDF(mat, normal, wo, wi);
    }
}

vec3 sampleF(in Material mat, in vec3 normal, in vec3 wo, in vec3 wi) {
    switch (mat.type) {
        case mat_Lambertian:    return lambertianF(mat, normal, wo, wi);
        case mat_GgxMetal:      return ggxMetalF(mat, normal, wo, wi);
        case mat_GgxPlastic:    return ggxPlasticF(mat, normal, wo, wi);
        default :               return lambertianF(DEFAULT_MATERIAL, normal, wo, wi);
    }
}

void sampleBSDF(in Material mat, in Hit hit, in vec3 wo, out SampleResult result, inout uint seed) {
    switch (mat.type) {
        case mat_Lambertian:    sampleLambertianBSDF(mat, hit, wo, result, seed); break;
        case mat_GgxMetal:      sampleGgxMetalBSDF(mat, hit, wo, result, seed); break;
        case mat_GgxPlastic:    sampleGgxPlasticBSDF(mat, hit, wo, result, seed); break;
        default :               sampleLambertianBSDF(DEFAULT_MATERIAL, hit, wo, result, seed); break;
    }
}

#endif
