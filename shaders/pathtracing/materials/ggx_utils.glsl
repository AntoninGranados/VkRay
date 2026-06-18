#ifndef GGX_UTILS_GLSL
#define GGX_UTILS_GLSL

#include "../utils.glsl"
#include "../random.glsl"

#include "material_utils.glsl"

vec3 ggxScatter(in Material mat, in Hit hit, in vec3 wo, in float alpha, out vec3 halfVector, inout uint seed) {
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
    if (abs(hit.normal.z) < 1 - EPS) {
        t = normalize(cross(vec3(0.0, 0.0, 1.0), hit.normal));
    } else {
        t = normalize(cross(vec3(0.0, 1.0, 0.0), hit.normal));
    }
    vec3 b = cross(hit.normal, t);

    halfVector = normalize(local.x * t + local.y * b + local.z * hit.normal);
    return reflect(-wo, halfVector);
}

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

struct GgxTerms {
    float D;
    float G;
    float NoM;
    float VoM;
    float cosWo;
    float cosWi;
};

GgxTerms computeGgxTerms(float alpha, in Hit hit, in vec3 wo, in vec3 wi, in vec3 m) {
    GgxTerms t;
    t.NoM   = max(dot(hit.normal, m), EPS);
    t.VoM   = max(dot(wo, m), EPS);
    t.cosWo = max(abs(dot(hit.normal, wo)), EPS);
    t.cosWi = max(abs(dot(hit.normal, wi)), EPS);
    t.D     = D_ggx(alpha, hit.normal, m);
    t.G     = G1_ggx(alpha, hit.normal, wo) * G1_ggx(alpha, hit.normal, wi);
    return t;
}

vec3 ggxBRDF(in GgxTerms t) {
    return vec3(t.D * t.G / (4.0 * t.cosWo * t.cosWi + EPS));
}

float ggxPDF(in GgxTerms t) {
    return (t.D * t.NoM) / (4.0 * t.VoM);
}


#endif // GGX_UTILS_GLSL
