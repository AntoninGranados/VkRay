#ifndef GGX_GLOSSY_GLSL
#define GGX_GLOSSY_GLSL

#include "../utils.glsl"
#include "../random.glsl"

#include "material_utils.glsl"
#include "lambertian.glsl"
#include "ggx_utils.glsl"

BSDFEval evalGgxGlossyBSDF(in Material mat, in Hit hit, in vec3 wo, in vec3 wi) {
    if (dot(hit.normal, wi) <= 0.0) return BSDFEval(vec3(0.0), 0.0);

    float alpha = max(mat.roughness * mat.roughness, EPS_HIGH);
    vec3 m = normalize(wo + wi);
    GgxTerms t = computeGgxTerms(alpha, hit, wo, wi, m);

    float F_VoH = fresnelDielectric(t.VoH,  1.0, mat.ior);
    float F_NoL = fresnelDielectric(t.cosWi, 1.0, mat.ior);
    float F_NoV = fresnelDielectric(t.cosWo, 1.0, mat.ior);

    vec3 fSpec = F_VoH * ggxBRDF(t);
    vec3 fDiff = (1.0 - F_NoL) * (1.0 - F_NoV) * mat.albedo / PI;

    return BSDFEval(
        fSpec + fDiff,
        F_NoV * ggxPDF(t) + (1.0 - F_NoV) * (t.cosWi / PI)
    );
}

BSDFSample sampleGgxGlossyBSDF(in Material mat, in Hit hit, in vec3 wo, inout uint seed) {
    float alpha = max(mat.roughness * mat.roughness, EPS_HIGH);

    float NoV = max(dot(hit.normal, wo), 0.0);
    float pSpec = fresnelDielectric(NoV, 1.0, mat.ior);

    float xi = rand(seed);

    vec3 wi;
    if (xi < pSpec) {
        vec3 v;
        wi = ggxScatter(mat, hit, wo, alpha, v, seed);
    } else {
        wi = cosineScatter(mat, hit.normal, wo, seed);
    }

    BSDFEval eval = evalGgxGlossyBSDF(mat, hit, wo, wi);

    BSDFSample bsdf;
    float cosB = abs(dot(hit.normal, wi));
    bsdf.wi      = wi;
    bsdf.weight  = eval.f * cosB / max(eval.pdf, EPS);
    bsdf.pdf     = eval.pdf;
    bsdf.isDelta = false;
    bsdf.medium.isInside = false;
    return bsdf;
}

#endif // GGX_GLOSSY_GLSL
