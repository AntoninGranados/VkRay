import "../scene.slang";
import "../utils.slang";
import "../random/utils.slang";

import "material_utils.slang";
import "bsdf.slang";
import "ggx_utils.slang";

struct MetalBsdf : IBsdf {
    float3 albedo;
    float roughness;

    BSDFEval eval(Hit hit, float3 wo, float3 wi) {
        float alpha = max(roughness * roughness, EPS_HIGH);
        float3 h = normalize(wo + wi);
        GgxTerms t = computeGgxTerms(alpha, hit, wo, wi, h);
        float3 F = schlickAlbedo(t.VoH, albedo);
        return BSDFEval(
            F * ggxBRDF(t),
            ggxPDF(t)
        );
    }

    BSDFSample sample(Hit hit, float3 wo, inout RngState rng) {
        float alpha = max(roughness * roughness, EPS_HIGH);
        float3 h;
        float3 wi = ggxScatter(hit, wo, alpha, h, rng);
        GgxTerms t = computeGgxTerms(alpha, hit, wo, wi, h);
        float3 F = schlickAlbedo(t.VoH, albedo);

        return BSDFSample(F * t.G * t.VoH / (t.cosWo * t.NoH), wi, ggxPDF(t), false, NO_MEDIUM);
    }

    BsdfProperties properties() {
        return BsdfProperties(mat_Metal, albedo, roughness);
    }
}

struct MetalMaterial : IMaterial {
    uint base;

    IBsdf evaluate(inout Hit hit, float3 wo, inout RngState rng) {
        MetalBsdf bsdf = loadParams<MetalBsdf>(base);
        bsdf.albedo *= hit.vertexColor;
        return bsdf;
    }
}
