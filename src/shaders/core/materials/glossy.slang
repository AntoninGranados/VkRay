import "../utils.slang";
import "../random/utils.slang";

import "material_utils.slang";
import "diffuse.slang";
import "ggx_utils.slang";

BSDFEval evalGlossyBSDF(in ResolvedMaterial mat, in Hit hit, in float3 wo, in float3 wi) {
    if (dot(hit.normal, wi) <= 0.0) return BSDFEval(float3(0.0), 0.0);

    float alpha = max(mat.glossyRoughness * mat.glossyRoughness, EPS_HIGH);
    float3 m = normalize(wo + wi);
    GgxTerms t = computeGgxTerms(alpha, hit, wo, wi, m);

    float F_VoH = fresnelDielectric(t.VoH,  1.0, mat.glossyIor);
    float F_NoL = fresnelDielectric(t.cosWi, 1.0, mat.glossyIor);
    float F_NoV = fresnelDielectric(t.cosWo, 1.0, mat.glossyIor);

    float3 fSpec = F_VoH * ggxBRDF(t);
    float3 fDiff = (1.0 - F_NoL) * (1.0 - F_NoV) * mat.albedo / PI;

    return BSDFEval(
        fSpec + fDiff,
        F_NoV * ggxPDF(t) + (1.0 - F_NoV) * (t.cosWi / PI)
    );
}

BSDFSample sampleGlossyBSDF(in ResolvedMaterial mat, in Hit hit, in float3 wo, inout RngState rng) {
    float alpha = max(mat.glossyRoughness * mat.glossyRoughness, EPS_HIGH);

    float NoV = max(dot(hit.normal, wo), 0.0);
    float pSpec = fresnelDielectric(NoV, 1.0, mat.glossyIor);

    float xi = rand(rng);

    float3 wi;
    if (xi < pSpec) {
        float3 v;
        wi = ggxScatter(mat, hit, wo, alpha, v, rng);
    } else {
        wi = cosineScatter(mat, hit.normal, wo, rng);
    }

    BSDFEval eval = evalGlossyBSDF(mat, hit, wo, wi);

    BSDFSample bsdf;
    float cosB = abs(dot(hit.normal, wi));
    bsdf.wi      = wi;
    bsdf.weight  = eval.f * cosB / max(eval.pdf, EPS);
    bsdf.pdf     = eval.pdf;
    bsdf.isDelta = false;
    bsdf.medium.isDielectric = false;
    bsdf.medium.isVolume = false;
    return bsdf;
}
