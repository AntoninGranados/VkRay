#ifndef PRINCIPLED_GLSL
#define PRINCIPLED_GLSL

#include "../utils.glsl"
#include "../random.glsl"

#include "material_utils.glsl"
#include "ggx_metal.glsl"
#include "ggx_glossy.glsl"
#include "dielectric.glsl"

BSDFEval evalPrincipledBSDF(in Material mat, in Hit hit, in vec3 wo, in vec3 wi) {
    float pMetal = mat.metalness;
    float pTrans = (1.0 - pMetal) * mat.transmission;
    float pGlossy = 1.0 - pMetal - pTrans;

    BSDFEval evalMetal = evalGgxMetalBSDF(mat, hit, wo, wi);
    BSDFEval evalGlossy = evalGgxGlossyBSDF(mat, hit, wo, wi);

    return BSDFEval(
        pMetal * evalMetal.f + pGlossy * evalGlossy.f,
        pMetal * evalMetal.pdf + pGlossy * evalGlossy.pdf
    );
}

BSDFSample samplePrincipledBSDF(in Material mat, in Hit hit, in vec3 wo, inout uint seed) {
    float pMetal = mat.metalness;
    float pTrans = (1.0 - pMetal) * mat.transmission;
    float pGlossy = 1.0 - pMetal - pTrans;

    float r = rand(seed);
    BSDFSample bsdf;
    if (r < pMetal) {
        bsdf = sampleGgxMetalBSDF(mat, hit, wo, seed);
    } else if (r < pMetal + pTrans) {
        bsdf = sampleDielectricBSDF(mat, hit, wo, seed);
    } else {
        bsdf = sampleGgxGlossyBSDF(mat, hit, wo, seed);
    }

    if (!bsdf.isDelta) {
        BSDFEval eval = evalPrincipledBSDF(mat, hit, wo, bsdf.wi);
        float cosB = abs(dot(hit.normal, bsdf.wi));
        bsdf.pdf    = eval.pdf;
        bsdf.weight = eval.f * cosB / max(eval.pdf, EPS);
    }

    bsdf.medium.isDielectric = (dot(bsdf.wi, hit.normal) < 0.0) == hit.frontFace && mat.transmission > 0.0;
    bsdf.medium.isVolume = false;
    bsdf.medium.absorption = mat.albedo;
    bsdf.medium.density = mat.density;
    return bsdf;
}

#endif // PRINCIPLED_GLSL