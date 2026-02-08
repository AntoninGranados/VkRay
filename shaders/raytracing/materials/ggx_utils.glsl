#ifndef GGX_UTILS_GLSL
#define GGX_UTILS_GLSL

#include "../utils.glsl"
#include "../random.glsl"

#include "material_utils.glsl"

vec3 ggxScatter(in Material mat, in Hit hit, in vec3 wo, in float alpha, inout uint seed) {
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

    vec3 m = normalize(local.x * t + local.y * b + local.z * hit.normal);
    return reflect(-wo, m);
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

float ggxPDF(in Material mat, in Hit hit, in vec3 wo, in vec3 wi) {
    vec3 m = normalize(wi + wo);
    float NoM = max(dot(hit.normal, m), 0.0);
    float VoM = max(dot(wo, m), 0.0);
    if (NoM <= 0.0 || VoM <= 0.0) return 0.0;

    float alpha = ggxMetalRoughness(mat) * ggxMetalRoughness(mat);
    float D = D_ggx(alpha, hit.normal, m);

    return (D * NoM) / (4.0 * VoM);
}

vec3 partialGgxF(in Material mat, in Hit hit, in vec3 wo, in vec3 wi, in float alpha) {
    // GGX f without the F (Fresnel) component
    vec3 m = normalize(wo + wi);
    
    float D = D_ggx(alpha, hit.normal, m);
    float G = G1_ggx(alpha, hit.normal, wo) * G1_ggx(alpha, hit.normal, wi);
    
    float cosWo = abs(dot(hit.normal, wo));
    float cosWi = abs(dot(hit.normal, wi));
    return mat.albedo * D * G / (4 * cosWo * cosWi + EPS);
}

#endif // GGX_UTILS_GLSL
