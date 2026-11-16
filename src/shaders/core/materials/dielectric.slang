import "../attributes.slang";
import "../scene.slang";
import "../utils.slang";
import "../random/utils.slang";

import "material_utils.slang";
import "bsdf.slang";

[Medium]
struct DielectricBsdf : IBsdf {
    [Range(0, 1)] [Color] [Animatable] float3 albedo = float3(1.0, 1.0, 1.0);
    [Range(0, 1)] [Animatable] float roughness = 0.0;
    [Range(1, 3)] [Animatable] float ior = 1.5;
    [Min(0)] [Animatable] float density = 0.0;
    [Range(0, 1)] [Animatable] float transmission = 1.0;
    [Range(-1, 1)] [Animatable] float anisotropic = 0.0;
    [Min(0)] int priority = 0;

    override bool isMediumBoundary() {
        return true;
    }

    MediumEntry mediumEntry(Object object) {
        return MediumEntry(mediumObjectKey(object), priority, ior, albedo, density, transmission, anisotropic);
    }

    BSDFEval eval(SurfacePoint surface, float3 wo, float3 wi) {
        return BSDFEval(
            float3(0.0),
            -1.0
        );
    }

    BSDFSample sample(SurfacePoint surface, float3 wo, inout RngState rng) {
        float etaI, etaT;
        interfaceIors(surface, ior, etaI, etaT);

        float3 normal = surface.normal + randomOnSphere(rng) * roughness;
        if (length(normal) < EPS) normal = surface.normal;
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

            crossInterface(mediumEntry(surface.object), surface);
        }

        bool entering = (dot(wi, surface.normal) < 0.0) == surface.frontFace;

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
