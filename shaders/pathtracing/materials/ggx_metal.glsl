#ifndef GGX_METAL_GLSL
#define GGX_METAL_GLSL

#include "../utils.glsl"
#include "../random.glsl"

#include "material_utils.glsl"
#include "ggx_utils.glsl"

BSDFEval evalGgxMetalBSDF(in Material mat, in Hit hit, in vec3 wo, in vec3 wi) {
    float alpha = mat.roughness * mat.roughness;
    vec3 m = normalize(wo + wi);
    GgxTerms t = computeGgxTerms(alpha, hit, wo, wi, m);
    vec3 F = schlickAlbedo(t.VoM, mat.albedo);
    return BSDFEval(F * ggxBRDF(t), ggxPDF(t));
}

BSDFSample sampleGgxMetalBSDF(in Material mat, in Hit hit, in vec3 wo, inout uint seed) {
    if (mat.roughness < 0.05) return sampleMirrorBSDF(mat.albedo, hit, wo);

    float alpha = mat.roughness * mat.roughness;
    vec3 m;
    vec3 wi = ggxScatter(mat, hit, wo, alpha, m, seed);
    GgxTerms t = computeGgxTerms(alpha, hit, wo, wi, m);
    vec3 F = schlickAlbedo(t.VoM, mat.albedo);

    BSDFSample bsdf;
    bsdf.wi      = wi;
    bsdf.weight  = F * t.G * t.VoM / (t.cosWo * t.NoM);
    bsdf.pdf     = ggxPDF(t);
    bsdf.isDelta = false;
    return bsdf;
}

#endif // GGX_METAL_GLSL
