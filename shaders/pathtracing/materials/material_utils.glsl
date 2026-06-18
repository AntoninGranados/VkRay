#ifndef MATERIAL_UTILS_GLSL
#define MATERIAL_UTILS_GLSL

#include "../utils.glsl"

struct BSDFSample {
    vec3 weight;
    vec3 wi;
    float pdf;
    bool isDelta;
};

struct BSDFEval {
    vec3 f;
    float pdf;
};

#define DEFAULT_MATERIAL Material(mat_Lambertian, vec3(1,0,1)*0.7, 0.0, 0.0, 0.0)

#define SCHLICK_APPROX(cosine, F0) F0 + (1-F0) * pow((1 - cosine), 5)

vec3 schlickIoR(float cosine, float ri) {
    float F0 = (1 - ri) / (1 + ri);
    F0 = F0*F0;
    return SCHLICK_APPROX(cosine, vec3(F0));
}

vec3 schlickAlbedo(float cosine, vec3 albedo) {
    return SCHLICK_APPROX(cosine, albedo);
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
    return bsdf;
}

#endif // MATERIAL_UTILS_GLSL
