import "../scene.slang";
import "../utils.slang";
import "../random/utils.slang";

import "material_utils.slang";
import "bsdf.slang";
import "metal.slang";
import "glossy.slang";
import "dielectric.slang";

struct PrincipledBsdf : IBsdf {
    float3 albedo;
    float roughness;
    float metalness;
    float ior;
    float transmission;
    float density;
    float anisotropic;
    float alpha;
    int priority;

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

    BSDFEval eval(Hit hit, float3 wo, float3 wi) {
        float pMetal  = metalness;
        float pTrans  = (1.0 - pMetal) * transmission;
        float pGlossy = 1.0 - pMetal - pTrans;

        BSDFEval eMetal  = metalProxy().eval(hit, wo, wi);
        BSDFEval eGlossy = glossyProxy().eval(hit, wo, wi);

        return BSDFEval(
            pMetal * eMetal.f + pGlossy * eGlossy.f,
            pMetal * eMetal.pdf + pGlossy * eGlossy.pdf
        );
    }

    BSDFSample sample(Hit hit, float3 wo, inout RngState rng) {
        if (alpha < rand(rng)) return BSDFSample(float3(1.0), -wo, 1.0, true, NO_MEDIUM);

        float pMetal  = metalness;
        float pTrans  = (1.0 - pMetal) * transmission;
        float pGlossy = 1.0 - pMetal - pTrans;

        float r = rand(rng);
        BSDFSample bsdf;
        if (r < pMetal) {
            bsdf = metalProxy().sample(hit, wo, rng);
            bsdf.medium.isDielectric = false;
            bsdf.medium.isVolume     = false;
        } else if (r < pMetal + pTrans) {
            bsdf = dielectricProxy().sample(hit, wo, rng);
            bsdf.medium.absorption    = albedo;
            bsdf.medium.density       = density;
            bsdf.medium.scatterAlbedo = 0.0;
            bsdf.medium.anisotropic   = anisotropic;
        } else {
            bsdf = glossyProxy().sample(hit, wo, rng);
        }

        if (!bsdf.isDelta) {
            BSDFEval eval = this.eval(hit, wo, bsdf.wi);
            float cosB = abs(dot(hit.normal, bsdf.wi));
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

struct PrincipledMaterial : IMaterial {
    uint base;

    IBsdf evaluate(inout Hit hit, float3 wo, inout RngState rng) {
        PrincipledBsdf bsdf = loadParams<PrincipledBsdf>(base);
        bsdf.albedo *= hit.vertexColor;
        return bsdf;
    }

    override bool isMediumBoundary() {
        return loadParams<PrincipledBsdf>(base).isTransmissive();
    }
}
