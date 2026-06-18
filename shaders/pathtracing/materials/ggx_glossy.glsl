#ifndef GGX_GLOSSY_GLSL
#define GGX_GLOSSY_GLSL

#include "../utils.glsl"
#include "../random.glsl"

#include "material_utils.glsl"
#include "lambertian.glsl"
#include "ggx_utils.glsl"

BSDFEval evalGgxGlossyBSDF(in Material mat, in Hit hit, in vec3 wo, in vec3 wi) {
    BSDFEval lambertianEval = evalLambertianBSDF(mat, hit, wo, wi);

    float alpha = mat.roughness * mat.roughness;
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
    bool specIsDelta = (mat.roughness < 0.05);

    float NoV = max(dot(hit.normal, wo), 0.0);
    vec3  Fv  = schlickIoR(NoV, mat.ior);
    float pSpec = luma(Fv);

    float xi = rand(seed);

    vec3 wi;
    if (xi < pSpec) {
        if (specIsDelta) {
            return sampleMirrorBSDF(vec3(1.0), hit, wo);
        } else {
            float alpha = mat.roughness * mat.roughness;
            vec3 v;
            wi = ggxScatter(mat, hit, wo, alpha, v, seed);
        }
    } else {
        wi = cosineScatter(mat, hit.normal, wo, seed);
    }

    BSDFEval eval = evalGgxGlossyBSDF(mat, hit, wo, wi);

    BSDFSample bsdf;
    float cosB = abs(dot(hit.normal, wi));
    bsdf.wi      = wi;
    bsdf.weight  = eval.f * cosB / eval.pdf;
    bsdf.pdf     = eval.pdf;
    bsdf.isDelta = false;
    return bsdf;
}

#endif // GGX_GLOSSY_GLSL
