import "../attributes.slang";
import "../scene.slang";
import "../utils.slang";
import "../random/utils.slang";

import "material_utils.slang";
import "bsdf.slang";
import "diffuse.slang";
import "ggx_utils.slang";

struct GlossyBsdf : IBsdf {
    [Range(0, 1)] [Color] [Animatable] float3 albedo = float3(0.8, 0.8, 0.8);
    [Range(0, 1)] [Animatable] float roughness = 0.5;
    [Range(1, 3)] [Animatable] float ior = 1.5;

    BSDFEval eval(SurfacePoint surface, float3 wo, float3 wi) {
        float diffuseCos = max(dot(surface.normal, wi), 0.0);

        float alpha = max(roughness * roughness, EPS_HIGH);
        float3 m = normalize(wo + wi);
        GgxTerms t = computeGgxTerms(alpha, surface, wo, wi, m);

        float F_VoH = fresnelDielectric(t.VoH, 1.0, ior);
        float specularAlbedo = ggxDirectionalAlbedo(roughness, max(dot(surface.normal, wo), 0.0), ior);

        float3 fSpec = F_VoH * ggxBRDF(t);
        float3 fDiff = (1.0 - specularAlbedo) * albedo / PI;

        return BSDFEval(
            fSpec + (diffuseCos > 0.0 ? fDiff : float3(0.0)),
            specularAlbedo * ggxPDF(t) + (1.0 - specularAlbedo) * (diffuseCos / PI)
        );
    }

    BSDFSample sample(SurfacePoint surface, float3 wo, inout RngState rng) {
        float alpha = max(roughness * roughness, EPS_HIGH);

        float pSpec = ggxDirectionalAlbedo(roughness, max(dot(surface.normal, wo), 0.0), ior);

        float xi = rand(rng);

        float3 wi;
        if (xi < pSpec) {
            float3 v;
            wi = ggxScatter(surface, wo, alpha, v, rng);
        } else {
            wi = cosineScatter(surface.normal, rng);
        }

        BSDFEval eval = this.eval(surface, wo, wi);

        float cosB = abs(dot(surface.normal, wi));
        return BSDFSample(eval.f * cosB / max(eval.pdf, EPS), wi, eval.pdf, false, NO_MEDIUM);
    }

    BsdfProperties properties() {
        return BsdfProperties(mat_Glossy, albedo, roughness);
    }
}
