#ifndef GLOSSY_GLSL
#define GLOSSY_GLSL

#include "../utils.glsl"
#include "../random/utils.glsl"

#include "material_utils.glsl"
#include "diffuse.glsl"
#include "ggx_utils.glsl"

BSDFEval evalGlossyBSDF(in ResolvedMaterial mat, in Hit hit, in vec3 wo, in vec3 wi) {
    if (dot(hit.normal, wi) <= 0.0) return BSDFEval(vec3(0.0), 0.0);

    float alpha = max(glossyRoughness(mat) * glossyRoughness(mat), EPS_HIGH);
    vec3 m = normalize(wo + wi);
    GgxTerms t = computeGgxTerms(alpha, hit, wo, wi, m);

    float F_VoH = fresnelDielectric(t.VoH,  1.0, glossyIor(mat));
    float F_NoL = fresnelDielectric(t.cosWi, 1.0, glossyIor(mat));
    float F_NoV = fresnelDielectric(t.cosWo, 1.0, glossyIor(mat));

    vec3 fSpec = F_VoH * ggxBRDF(t);
    vec3 fDiff = (1.0 - F_NoL) * (1.0 - F_NoV) * albedo(mat) / PI;

    return BSDFEval(
        fSpec + fDiff,
        F_NoV * ggxPDF(t) + (1.0 - F_NoV) * (t.cosWi / PI)
    );
}

BSDFSample sampleGlossyBSDF(in ResolvedMaterial mat, in Hit hit, in vec3 wo, inout RngState rng) {
    float alpha = max(glossyRoughness(mat) * glossyRoughness(mat), EPS_HIGH);

    float NoV = max(dot(hit.normal, wo), 0.0);
    float pSpec = fresnelDielectric(NoV, 1.0, glossyIor(mat));

    float xi = rand(rng);

    vec3 wi;
    if (xi < pSpec) {
        vec3 v;
        wi = ggxScatter(mat, hit, wo, alpha, v, rng);
    } else {
        wi = cosineScatter(mat, hit.normal, wo, rng);
    }

    BSDFEval eval = evalGlossyBSDF(mat, hit, wo, wi);

    BSDFSample bsdf;
    float cosB = abs(dot(hit.normal, wi));
    bsdf.wi      = wi;
    bsdf.weight  = eval.f * cosB / max(eval.pdf, EPS);
    bsdf.pdf     = eval.pdf;
    bsdf.isDelta = false;
    bsdf.medium.isDielectric = false;
    bsdf.medium.isVolume = false;
    return bsdf;
}

#endif // GLOSSY_GLSL
