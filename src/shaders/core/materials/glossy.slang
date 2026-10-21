import "../scene.slang";
import "../utils.slang";
import "../random/utils.slang";

import "material_utils.slang";
import "bsdf.slang";
import "diffuse.slang";
import "ggx_utils.slang";

struct GlossyBsdf : IBsdf {
    float3 albedo;
    float roughness;
    float ior;

    BSDFEval eval(Hit hit, float3 wo, float3 wi) {
        if (dot(hit.normal, wi) <= 0.0) return BSDFEval(float3(0.0), 0.0);

        float alpha = max(roughness * roughness, EPS_HIGH);
        float3 m = normalize(wo + wi);
        GgxTerms t = computeGgxTerms(alpha, hit, wo, wi, m);

        float F_VoH = fresnelDielectric(t.VoH,  1.0, ior);
        float F_NoL = fresnelDielectric(t.cosWi, 1.0, ior);
        float F_NoV = fresnelDielectric(t.cosWo, 1.0, ior);

        float3 fSpec = F_VoH * ggxBRDF(t);
        float3 fDiff = (1.0 - F_NoL) * (1.0 - F_NoV) * albedo / PI;

        return BSDFEval(
            fSpec + fDiff,
            F_NoV * ggxPDF(t) + (1.0 - F_NoV) * (t.cosWi / PI)
        );
    }

    BSDFSample sample(Hit hit, float3 wo, inout RngState rng) {
        float alpha = max(roughness * roughness, EPS_HIGH);

        float NoV = max(dot(hit.normal, wo), 0.0);
        float pSpec = fresnelDielectric(NoV, 1.0, ior);

        float xi = rand(rng);

        float3 wi;
        if (xi < pSpec) {
            float3 v;
            wi = ggxScatter(hit, wo, alpha, v, rng);
        } else {
            wi = cosineScatter(hit.normal, rng);
        }

        BSDFEval eval = this.eval(hit, wo, wi);

        float cosB = abs(dot(hit.normal, wi));
        return BSDFSample(eval.f * cosB / max(eval.pdf, EPS), wi, eval.pdf, false, NO_MEDIUM);
    }

    BsdfProperties properties() {
        return BsdfProperties(mat_Glossy, albedo, roughness);
    }
}

struct GlossyMaterial : IMaterial {
    uint base;

    IBsdf evaluate(inout Hit hit, float3 wo, inout RngState rng) {
        GlossyBsdf bsdf = loadParams<GlossyBsdf>(base);
        bsdf.albedo *= hit.vertexColor;
        return bsdf;
    }
}
