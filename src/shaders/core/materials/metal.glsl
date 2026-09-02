#ifndef METAL_GLSL
#define METAL_GLSL

#include "../utils.glsl"
#include "../random/utils.glsl"

#include "material_utils.glsl"
#include "ggx_utils.glsl"

BSDFEval evalMetalBSDF(in Material mat, in Hit hit, in vec3 wo, in vec3 wi) {
    float alpha = max(metalRoughness(mat) * metalRoughness(mat), EPS_HIGH);
    vec3 h = normalize(wo + wi);
    GgxTerms t = computeGgxTerms(alpha, hit, wo, wi, h);
    vec3 F = schlickAlbedo(t.VoH, albedo(mat));
    return BSDFEval(
        F * ggxBRDF(t),
        ggxPDF(t)
    );
}

BSDFSample sampleMetalBSDF(in Material mat, in Hit hit, in vec3 wo, inout RngState rng) {
    float alpha = max(metalRoughness(mat) * metalRoughness(mat), EPS_HIGH);
    vec3 h;
    vec3 wi = ggxScatter(mat, hit, wo, alpha, h, rng);
    GgxTerms t = computeGgxTerms(alpha, hit, wo, wi, h);
    vec3 F = schlickAlbedo(t.VoH, albedo(mat));

    BSDFSample bsdf;
    bsdf.wi      = wi;
    bsdf.weight  = F * t.G * t.VoH / (t.cosWo * t.NoH);
    bsdf.pdf     = ggxPDF(t);
    bsdf.isDelta = false;
    bsdf.medium.isDielectric = false;
    bsdf.medium.isVolume = false;
    return bsdf;
}

#endif // METAL_GLSL
