import "../scene.slang";
import "../utils.slang";
import "../random/utils.slang";

import "material_utils.slang";
import "bsdf.slang";

struct DielectricBsdf : IBsdf {
    float3 albedo;
    float roughness;
    float ior;
    float density;
    float transmission;
    float anisotropic;
    int priority;

    MediumEntry mediumEntry(Object object) {
        return MediumEntry(mediumObjectKey(object), priority, ior, albedo, density, transmission, anisotropic);
    }

    BSDFEval eval(Hit hit, float3 wo, float3 wi) {
        return BSDFEval(
            float3(0.0),
            -1.0
        );
    }

    BSDFSample sample(Hit hit, float3 wo, inout RngState rng) {
        float etaI, etaT;
        interfaceIors(hit, ior, etaI, etaT);

        float3 normal = hit.normal + randomOnSphere(rng) * roughness;
        if (length(normal) < EPS) normal = hit.normal;
        else normal = normalize(normal);

        float eta = etaI / etaT;

        float cosTheta = clamp(dot(wo, normal), 0.0, 1.0);
        float F = fresnelDielectric(cosTheta, etaI, etaT);

        float3 wi;
        float3 weight;
        float pdf;
        if (rand(rng) < F) {
            wi = reflect(-wo, normal);

            pdf = max(F, EPS);
            weight = float3(1.0);
        } else {
            wi = refract(-wo, normal, eta);
            float etaFactor = (etaI * etaI) / (etaT * etaT);

            pdf = max(1.0 - F, EPS);
            weight = float3(etaFactor);

            crossInterface(mediumEntry(hit.object), hit);
        }

        bool entering = (dot(wi, hit.normal) < 0.0) == hit.frontFace;

        BSDFSample bsdf;
        bsdf.wi                   = wi;
        bsdf.weight               = weight;
        bsdf.pdf                  = pdf;
        bsdf.isDelta              = true;
        bsdf.medium.isDielectric  = entering;
        bsdf.medium.isVolume      = entering && density > 0.0;
        bsdf.medium.absorption    = albedo;
        bsdf.medium.density       = density;
        bsdf.medium.scatterAlbedo = transmission;
        bsdf.medium.anisotropic   = anisotropic;
        return bsdf;
    }

    BsdfProperties properties() {
        return BsdfProperties(mat_Dielectric, albedo, roughness);
    }

    override Optional<MediumEntry> medium(Object object) {
        return mediumEntry(object);
    }
}

struct DielectricMaterial : IMaterial {
    uint base;

    IBsdf evaluate(inout Hit hit, float3 wo, inout RngState rng) {
        DielectricBsdf bsdf = loadParams<DielectricBsdf>(base);
        bsdf.albedo *= hit.vertexColor;
        return bsdf;
    }

    override bool isMediumBoundary() {
        return true;
    }
}
