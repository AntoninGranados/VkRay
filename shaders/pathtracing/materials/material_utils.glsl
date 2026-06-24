#ifndef MATERIAL_UTILS_GLSL
#define MATERIAL_UTILS_GLSL

#include "../utils.glsl"

struct BSDFMediumInfo {
    bool isInside;
    vec3 absorption;
    float density;
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

#define DEFAULT_MATERIAL Material(mat_Lambertian, vec3(1,0,1)*0.7, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0)

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

// Returns the expected scalar transmission factor through a surface for shadow rays.
// Returns -1.0 if the material is fully opaque (shadow ray should be blocked).
float shadowTransmissionFactor(in Material mat, in Hit shadowHit, in vec3 wi) {
    float cosI = abs(dot(wi, shadowHit.normal));
    float etaI = shadowHit.frontFace ? 1.0 : mat.ior;
    float etaT = shadowHit.frontFace ? mat.ior : 1.0;
    float T = 1.0 - fresnelDielectric(cosI, etaI, etaT);

    if (mat.type == mat_Dielectric) {
        return T;
    }
    if (mat.type == mat_Principled) {
        float pTrans = (1.0 - mat.metalness) * mat.transmission;
        if (pTrans > EPS) return pTrans * T;
    }
    return -1.0;
}

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
    bsdf.medium.isInside = false;
    return bsdf;
}

#endif // MATERIAL_UTILS_GLSL
