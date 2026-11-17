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
    [Texture] int albedoTexture = 0;
    [Range(0, 1)] [Animatable] float roughness = 0.5;
    [Texture] int roughnessTexture = 0;
    [Range(0, 1)] [Animatable] float metalness = 0.0;
    [Texture] int metalnessTexture = 0;
    [Texture] int normalTexture = 0;
    [Min(0)] [Color] [Animatable] float3 emission = float3(0.0);
    [Texture] int emissionTexture = 0;
    [Min(0)] [Animatable] float emissionStrength = 1.0;
    [Range(1, 3)] [Animatable] float ior = 1.5;
    [Range(0, 1)] [Animatable] float transmission = 0.0;
    [Min(0)] [Animatable] float density = 0.0;
    [Range(-1, 1)] [Animatable] float anisotropic = 0.0;
    [Range(0, 1)] [Animatable] float alpha = 1.0;
    [Min(0)] int priority = 0;

    [mutating] override void shade(inout SurfacePoint surface) {
        if (albedoTexture != 0) {
            float4 base = sampleTexture(albedoTexture, surface.uv);
            albedo = base.rgb;
            alpha *= base.a;
        }
        if (roughnessTexture != 0) roughness = sampleTexture(roughnessTexture, surface.uv).g;
        if (metalnessTexture != 0) metalness = sampleTexture(metalnessTexture, surface.uv).b;
        if (emissionTexture != 0) emission = sampleTexture(emissionTexture, surface.uv).rgb;
        if (normalTexture != 0 && surface.tangent.w != 0.0) {
            float3 normal = surface.frontFace ? surface.normal : -surface.normal;
            float3 tangent = surface.tangent.xyz - normal * dot(normal, surface.tangent.xyz);
            float tangentLength = length(tangent);
            if (!(tangentLength > EPS)) return;
            tangent /= tangentLength;
            float3 bitangent = cross(normal, tangent) * surface.tangent.w;
            float3 local = sampleTexture(normalTexture, surface.uv).xyz * 2.0 - 1.0;
            normal = normalize(local.x * tangent + local.y * bitangent + local.z * normal);
            normal = surface.frontFace ? normal : -normal;
            surface.normal = normal;
        }
    }

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
        return BsdfProperties(mat_Principled, albedo, roughness, emission * emissionStrength);
    }

    override Optional<MediumEntry> medium(Object object) {
        if (!isTransmissive()) return none;
        return dielectricProxy().mediumEntry(object);
    }
}
