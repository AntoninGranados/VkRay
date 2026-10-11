import "../utils.slang";
import "../random/utils.slang";

import "material_utils.slang";
import "metal.slang";
import "glossy.slang";
import "dielectric.slang";

ResolvedMaterial principledGlossyProxy(in ResolvedMaterial mat) {
    return Glossy(mat.albedo, mat.principledRoughness, mat.principledIor);
}

ResolvedMaterial principledDielectricProxy(in ResolvedMaterial mat) {
    ResolvedMaterial proxy = Dielectric(
        mat.albedo,
        mat.principledRoughness,
        mat.principledIor,
        0.0,
        mat.principledDensity,
        mat.principledAnisotropic
    );
    proxy.dielectricPriority = mat.principledPriority;
    return proxy;
}

BSDFEval evalPrincipledBSDF(in ResolvedMaterial mat, in Hit hit, in float3 wo, in float3 wi) {
    float pMetal  = mat.principledMetalness;
    float pTrans  = (1.0 - pMetal) * mat.principledTransmission;
    float pGlossy = 1.0 - pMetal - pTrans;

    BSDFEval eMetal  = evalMetalBSDF(mat, hit, wo, wi);
    BSDFEval eGlossy = evalGlossyBSDF(principledGlossyProxy(mat), hit, wo, wi);

    return BSDFEval(
        pMetal * eMetal.f + pGlossy * eGlossy.f,
        pMetal * eMetal.pdf + pGlossy * eGlossy.pdf
    );
}

BSDFSample samplePrincipledBSDF(in ResolvedMaterial mat, in Hit hit, in float3 wo, inout RngState rng) {
    if (mat.principledAlpha < rand(rng)) {
        BSDFSample bsdf;
        bsdf.wi      = -wo;
        bsdf.pdf     = 1.0;
        bsdf.weight  = float3(1.0);
        bsdf.isDelta = true;
        bsdf.medium.isDielectric = false;
        bsdf.medium.isVolume = false;
        return bsdf;
    }

    float pMetal  = mat.principledMetalness;
    float pTrans  = (1.0 - pMetal) * mat.principledTransmission;
    float pGlossy = 1.0 - pMetal - pTrans;

    float r = rand(rng);
    BSDFSample bsdf;
    if (r < pMetal) {
        bsdf = sampleMetalBSDF(mat, hit, wo, rng);
        bsdf.medium.isDielectric = false;
        bsdf.medium.isVolume     = false;
    } else if (r < pMetal + pTrans) {
        bsdf = sampleDielectricBSDF(principledDielectricProxy(mat), hit, wo, rng);
        bsdf.medium.absorption    = mat.albedo;
        bsdf.medium.density       = mat.principledDensity;
        bsdf.medium.scatterAlbedo = 0.0;
        bsdf.medium.anisotropic   = mat.principledAnisotropic;
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
