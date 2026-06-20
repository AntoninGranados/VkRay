#ifndef GGX_GLOSSY_GLSL
#define GGX_GLOSSY_GLSL

#include "../utils.glsl"
#include "../random.glsl"

#include "material_utils.glsl"
#include "lambertian.glsl"
#include "ggx_utils.glsl"

BSDFEval evalGgxGlossyBSDF(in Material mat, in Hit hit, in vec3 wo, in vec3 wi) {
    BSDFEval lambertianEval = evalLambertianBSDF(mat, hit, wo, wi);

    float alpha = max(mat.roughness * mat.roughness, 1e-6);
    vec3 m = normalize(wo + wi);
    GgxTerms t = computeGgxTerms(alpha, hit, wo, wi, m);
    float NoV = max(dot(hit.normal, wo), 0.0);
    vec3 F = schlickIoR(NoV, mat.ior);

    float pSpec = luma(F);
    vec3 fSpec = F * ggxBRDF(t);
    vec3 fDiff = (vec3(1.0) - F) * lambertianEval.f;

    return BSDFEval(
        fSpec + fDiff,
        pSpec * ggxPDF(t) + (1.0 - pSpec) * lambertianEval.pdf
    );
}

BSDFSample sampleGgxGlossyBSDF(in Material mat, in Hit hit, in vec3 wo, inout uint seed) {
    float alpha = max(mat.roughness * mat.roughness, 1e-6);

    float NoV = max(dot(hit.normal, wo), 0.0);
    vec3  Fv  = schlickIoR(NoV, mat.ior);
    float pSpec = luma(Fv);

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
    return bsdf;
}

#endif // GGX_GLOSSY_GLSL
