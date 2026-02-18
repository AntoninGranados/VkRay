#ifndef GGX_METAL_GLSL
#define GGX_METAL_GLSL

#include "../utils.glsl"
#include "../random.glsl"

#include "material_utils.glsl"
#include "ggx_utils.glsl"

float ggxMetalPDF(in Material mat, in Hit hit, in vec3 wo, in vec3 wi) {
    return ggxPDF(mat, hit, wo, wi);
}

vec3 ggxMetalF(in Material mat, in Hit hit, in vec3 wo, in vec3 wi) {
    float alpha = ggxMetalRoughness(mat) * ggxMetalRoughness(mat);
    vec3 m = normalize(wo + wi);
    vec3 F = schlickAlbedo(dot(wo, m), mat.albedo);
    return F * partialGgxF(mat, hit, wo, wi, alpha);
}

void sampleGgxMetalBSDF(in Material mat, in Hit hit, in vec3 wo, out SampleResult result, inout uint seed) {
    if (ggxMetalRoughness(mat) < 0.05) {
        sampleMirrorBSDF(mat.albedo, hit, wo, result);
    } else {
        float alpha = ggxMetalRoughness(mat) * ggxMetalRoughness(mat);
        result.wi = ggxScatter(mat, hit, wo, alpha, seed);

        result.f = ggxMetalF(mat, hit, wo, result.wi);
        result.pdf = ggxPDF(mat, hit, wo, result.wi);
        result.isDelta = ggxMetalRoughness(mat) < EPS;
    }
}

#endif // GGX_METAL_GLSL
