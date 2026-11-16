import "../attributes.slang";
import "../scene.slang";
import "../utils.slang";
import "../random/utils.slang";

import "material_utils.slang";
import "bsdf.slang";
import "metal.slang";
import "glossy.slang";
import "dielectric.slang";

[Medium]
struct PrincipledBsdf : IBsdf {
    [Range(0, 1)] [Color] [Animatable] float3 albedo = float3(0.8, 0.8, 0.8);
    [Range(0, 1)] [Animatable] float roughness = 0.5;
    [Range(0, 1)] [Animatable] float metalness = 0.0;
    [Range(1, 3)] [Animatable] float ior = 1.5;
    [Range(0, 1)] [Animatable] float transmission = 0.0;
    [Min(0)] [Animatable] float density = 0.0;
    [Range(-1, 1)] [Animatable] float anisotropic = 0.0;
    [Range(0, 1)] [Animatable] float alpha = 1.0;
    [Min(0)] int priority = 0;

    MetalBsdf metalProxy() {
        return MetalBsdf(albedo, roughness);
    }

    GlossyBsdf glossyProxy() {
        return GlossyBsdf(albedo, roughness, ior);
    }

    DielectricBsdf dielectricProxy() {
        return DielectricBsdf(albedo, roughness, ior, density, 0.0, anisotropic, priority);
    }

    bool isTransmissive() {
        return (1.0 - metalness) * transmission > EPS;
    }

    override bool isMediumBoundary() {
        return isTransmissive();
    }

    BSDFEval eval(SurfacePoint surface, float3 wo, float3 wi) {
        float pMetal  = metalness;
        float pTrans  = (1.0 - pMetal) * transmission;
        float pGlossy = 1.0 - pMetal - pTrans;

        BSDFEval eMetal  = metalProxy().eval(surface, wo, wi);
        BSDFEval eGlossy = glossyProxy().eval(surface, wo, wi);

        return BSDFEval(
            pMetal * eMetal.f + pGlossy * eGlossy.f,
            pMetal * eMetal.pdf + pGlossy * eGlossy.pdf
        );
    }

    BSDFSample sample(SurfacePoint surface, float3 wo, inout RngState rng) {
        if (alpha < rand(rng)) return BSDFSample(float3(1.0), -wo, 1.0, true, NO_MEDIUM);

        float pMetal  = metalness;
        float pTrans  = (1.0 - pMetal) * transmission;
        float pGlossy = 1.0 - pMetal - pTrans;

        float r = rand(rng);
        BSDFSample bsdf;
        if (r < pMetal) {
            bsdf = metalProxy().sample(surface, wo, rng);
            bsdf.medium.isDielectric = false;
            bsdf.medium.isVolume     = false;
        } else if (r < pMetal + pTrans) {
            bsdf = dielectricProxy().sample(surface, wo, rng);
            bsdf.medium.absorption    = albedo;
            bsdf.medium.density       = density;
            bsdf.medium.scatterAlbedo = 0.0;
            bsdf.medium.anisotropic   = anisotropic;
        } else {
            bsdf = glossyProxy().sample(surface, wo, rng);
        }

        if (!bsdf.isDelta) {
            BSDFEval eval = this.eval(surface, wo, bsdf.wi);
            float cosB = abs(dot(surface.normal, bsdf.wi));
            bsdf.pdf    = eval.pdf;
            bsdf.weight = eval.f * cosB / max(eval.pdf, EPS);
        }

        return bsdf;
    }

    BsdfProperties properties() {
        return BsdfProperties(mat_Principled, albedo, roughness);
    }

    override Optional<MediumEntry> medium(Object object) {
        if (!isTransmissive()) return none;
        return dielectricProxy().mediumEntry(object);
    }
}
