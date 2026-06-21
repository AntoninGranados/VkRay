#ifndef GGX_METAL_GLSL
#define GGX_METAL_GLSL

#include "../utils.glsl"
#include "../random.glsl"

#include "material_utils.glsl"
#include "ggx_utils.glsl"

BSDFEval evalGgxMetalBSDF(in Material mat, in Hit hit, in vec3 wo, in vec3 wi) {
    float alpha = max(mat.roughness * mat.roughness, EPS_HIGH);
    vec3 h = normalize(wo + wi);
    GgxTerms t = computeGgxTerms(alpha, hit, wo, wi, h);
    vec3 F = schlickAlbedo(t.VoH, mat.albedo);
    return BSDFEval(
        F * ggxBRDF(t),
        ggxPDF(t)
    );
}

BSDFSample sampleGgxMetalBSDF(in Material mat, in Hit hit, in vec3 wo, inout uint seed) {
    float alpha = max(mat.roughness * mat.roughness, EPS_HIGH);
    vec3 h;
    vec3 wi = ggxScatter(mat, hit, wo, alpha, h, seed);
    GgxTerms t = computeGgxTerms(alpha, hit, wo, wi, h);
    vec3 F = schlickAlbedo(t.VoH, mat.albedo);

    BSDFSample bsdf;
    bsdf.wi      = wi;
    bsdf.weight  = F * t.G * t.VoH / (t.cosWo * t.NoH);
    bsdf.pdf     = ggxPDF(t);
    bsdf.isDelta = false;
    bsdf.medium.isInside = false;
    return bsdf;
}

#endif // GGX_METAL_GLSL
