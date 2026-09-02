#ifndef PRINCIPLED_GLSL
#define PRINCIPLED_GLSL

#include "../utils.glsl"
#include "../random/utils.glsl"

#include "material_utils.glsl"
#include "metal.glsl"
#include "glossy.glsl"
#include "dielectric.glsl"

Material principledGlossyProxy(in Material mat) {
    return Glossy(albedo(mat), principledRoughness(mat), principledIor(mat));
}

Material principledDielectricProxy(in Material mat) {
    Material proxy;
    proxy.type = mat_Dielectric;
    setAlbedo(proxy, albedo(mat));
    proxy.payload[3] = principledRoughness(mat);
    proxy.payload[4] = principledIor(mat);
    proxy.payload[6] = principledDensity(mat);
    return proxy;
}

BSDFEval evalPrincipledBSDF(in Material mat, in Hit hit, in vec3 wo, in vec3 wi) {
    float pMetal  = principledMetalness(mat);
    float pTrans  = (1.0 - pMetal) * principledTransmission(mat);
    float pGlossy = 1.0 - pMetal - pTrans;

    BSDFEval eMetal  = evalMetalBSDF(mat, hit, wo, wi);
    BSDFEval eGlossy = evalGlossyBSDF(principledGlossyProxy(mat), hit, wo, wi);

    return BSDFEval(
        pMetal * eMetal.f + pGlossy * eGlossy.f,
        pMetal * eMetal.pdf + pGlossy * eGlossy.pdf
    );
}

BSDFSample samplePrincipledBSDF(in Material mat, in Hit hit, in vec3 wo, inout RngState rng) {
    if (principledAlpha(mat) < rand(rng)) {
        BSDFSample bsdf;
        bsdf.wi      = -wo;
        bsdf.pdf     = 1.0;
        bsdf.weight  = vec3(1.0);
        bsdf.isDelta = true;
        bsdf.medium.isDielectric = false;
        bsdf.medium.isVolume = false;
        return bsdf;
    }

    float pMetal  = principledMetalness(mat);
    float pTrans  = (1.0 - pMetal) * principledTransmission(mat);
    float pGlossy = 1.0 - pMetal - pTrans;

    float r = rand(rng);
    BSDFSample bsdf;
    if (r < pMetal) {
        bsdf = sampleMetalBSDF(mat, hit, wo, rng);
        bsdf.medium.isDielectric = false;
        bsdf.medium.isVolume     = false;
    } else if (r < pMetal + pTrans) {
        bsdf = sampleDielectricBSDF(principledDielectricProxy(mat), hit, wo, rng);
        bsdf.medium.absorption    = albedo(mat);
        bsdf.medium.density       = principledDensity(mat);
        bsdf.medium.scatterAlbedo = 0.0;
        bsdf.medium.anisotropic   = principledAnisotropic(mat);
    } else {
        bsdf = sampleGlossyBSDF(principledGlossyProxy(mat), hit, wo, rng);
    }

    if (!bsdf.isDelta) {
        BSDFEval eval = evalPrincipledBSDF(mat, hit, wo, bsdf.wi);
        float cosB = abs(dot(hit.normal, bsdf.wi));
        bsdf.pdf    = eval.pdf;
        bsdf.weight = eval.f * cosB / max(eval.pdf, EPS);
    }

    return bsdf;
}

#endif // PRINCIPLED_GLSL
