#ifndef PRINCIPLED_GLSL
#define PRINCIPLED_GLSL

#include "../utils.glsl"
#include "../random.glsl"

#include "material_utils.glsl"
#include "metal.glsl"
#include "glossy.glsl"
#include "dielectric.glsl"

Material principledGlossyProxy(in Material mat) {
    return mat_makeGlossy(mat_albedo(mat), mat_principled_roughness(mat), mat_principled_ior(mat));
}

Material principledDielectricProxy(in Material mat) {
    Material proxy;
    proxy.type = mat_Dielectric;
    mat_setAlbedo(proxy, mat_albedo(mat));
    proxy.payload[3] = mat_principled_roughness(mat);
    proxy.payload[4] = mat_principled_ior(mat);
    proxy.payload[6] = mat_principled_density(mat);
    return proxy;
}

BSDFEval evalPrincipledBSDF(in Material mat, in Hit hit, in vec3 wo, in vec3 wi) {
    float pMetal  = mat_principled_metalness(mat);
    float pTrans  = (1.0 - pMetal) * mat_principled_transmission(mat);
    float pGlossy = 1.0 - pMetal - pTrans;

    BSDFEval eMetal  = evalMetalBSDF(mat, hit, wo, wi);
    BSDFEval eGlossy = evalGlossyBSDF(principledGlossyProxy(mat), hit, wo, wi);

    return BSDFEval(
        pMetal * eMetal.f + pGlossy * eGlossy.f,
        pMetal * eMetal.pdf + pGlossy * eGlossy.pdf
    );
}

BSDFSample samplePrincipledBSDF(in Material mat, in Hit hit, in vec3 wo, inout uint seed) {
    if (mat_principled_alpha(mat) < rand(seed)) {
        BSDFSample bsdf;
        bsdf.wi      = -wo;
        bsdf.pdf     = 1.0;
        bsdf.weight  = vec3(1.0);
        bsdf.isDelta = true;
        bsdf.medium.isDielectric = false;
        bsdf.medium.isVolume = false;
        return bsdf;
    }

    float pMetal  = mat_principled_metalness(mat);
    float pTrans  = (1.0 - pMetal) * mat_principled_transmission(mat);
    float pGlossy = 1.0 - pMetal - pTrans;

    float r = rand(seed);
    BSDFSample bsdf;
    if (r < pMetal) {
        bsdf = sampleMetalBSDF(mat, hit, wo, seed);
        bsdf.medium.isDielectric = false;
        bsdf.medium.isVolume     = false;
    } else if (r < pMetal + pTrans) {
        bsdf = sampleDielectricBSDF(principledDielectricProxy(mat), hit, wo, seed);
        bsdf.medium.absorption    = mat_albedo(mat);
        bsdf.medium.density       = mat_principled_density(mat);
        bsdf.medium.scatterAlbedo = 0.0;
        bsdf.medium.anisotropic   = mat_principled_anisotropic(mat);
    } else {
        bsdf = sampleGlossyBSDF(principledGlossyProxy(mat), hit, wo, seed);
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
