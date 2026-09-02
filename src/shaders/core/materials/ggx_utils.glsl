#ifndef GGX_UTILS_GLSL
#define GGX_UTILS_GLSL

#include "../utils.glsl"
#include "../random/utils.glsl"

#include "material_utils.glsl"

vec3 ggxScatter(in Material mat, in Hit hit, in vec3 wo, in float alpha, out vec3 h, inout RngState rng) {
    float s1 = rand(rng);
    float s2 = rand(rng);

    float phi = 2.0 * PI * s1;
    float theta = atan(alpha * sqrt(s2 / (1.0 - s2)));

    vec3 local = vec3(
        sin(theta) * cos(phi),
        sin(theta) * sin(phi),
        cos(theta)
    );

    // Build tangent basis
    vec3 t;
    if (abs(hit.normal.z) < 1 - EPS) {
        t = normalize(cross(vec3(0.0, 0.0, 1.0), hit.normal));
    } else {
        t = normalize(cross(vec3(0.0, 1.0, 0.0), hit.normal));
    }
    vec3 b = cross(hit.normal, t);

    h = normalize(local.x * t + local.y * b + local.z * hit.normal);
    return reflect(-wo, h);
}

float D_ggx(in float alpha, in vec3 n, in vec3 h) {
    float NoH = max(dot(n, h), 0.0);
    if (NoH <= 0.0) return 0.0;

    float a_sq = alpha * alpha;
    float denom_part = NoH*NoH * (a_sq - 1.0) + 1.0;
    return a_sq / (PI * denom_part * denom_part);
}

float G1_ggx(in float alpha, in vec3 n, in vec3 v) {
    float NoV = dot(n, v);
    if (NoV <= 0.0) return 0.0;

    float a_sq = alpha * alpha;
    float denom = NoV + sqrt(a_sq + (1.0 - a_sq) * NoV * NoV);
    return 2.0 * NoV / denom;
}

struct GgxTerms {
    float D;
    float G;
    float NoH;
    float VoH;
    float cosWo;
    float cosWi;
};

GgxTerms computeGgxTerms(float alpha, in Hit hit, in vec3 wo, in vec3 wi, in vec3 h) {
    GgxTerms t;

    float NoH   = dot(hit.normal, h);
    float VoH   = dot(wo, h);
    float NoV   = dot(hit.normal, wo);
    float NoL   = dot(hit.normal, wi);

    if (NoH <= EPS || VoH <= EPS || NoV <= EPS || NoL <= EPS) {
        t.D = 0.0;
        t.G = 0.0;
        t.NoH = 0.0;
        t.VoH = 0.0;
        t.cosWo = 0.0;
        t.cosWi = 0.0;
        return t;
    }

    t.NoH   = NoH;
    t.VoH   = VoH;
    t.cosWo = NoV;
    t.cosWi = NoL;
    t.D     = D_ggx(alpha, hit.normal, h);
    t.G     = G1_ggx(alpha, hit.normal, wo) * G1_ggx(alpha, hit.normal, wi);
    return t;
}

vec3 ggxBRDF(in GgxTerms t) {
    if (t.D <= 0.0 || t.cosWo <= 0.0 || t.cosWi <= 0.0) return vec3(0.0);
    return vec3(t.D * t.G / (4.0 * t.cosWo * t.cosWi + EPS));
}

float ggxPDF(in GgxTerms t) {
    if (t.D <= 0.0 || t.VoH <= 0.0) return 0.0;
    return (t.D * t.NoH) / (4.0 * t.VoH);
}


#endif // GGX_UTILS_GLSL
