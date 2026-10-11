import "../utils.slang";
import "../random/utils.slang";

import "material_utils.slang";
import "ggx_utils.slang";

BSDFEval evalMetalBSDF(in ResolvedMaterial mat, in Hit hit, in float3 wo, in float3 wi) {
    float alpha = max(mat.metalRoughness * mat.metalRoughness, EPS_HIGH);
    float3 h = normalize(wo + wi);
    GgxTerms t = computeGgxTerms(alpha, hit, wo, wi, h);
    float3 F = schlickAlbedo(t.VoH, mat.albedo);
    return BSDFEval(
        F * ggxBRDF(t),
        ggxPDF(t)
    );
}

BSDFSample sampleMetalBSDF(in ResolvedMaterial mat, in Hit hit, in float3 wo, inout RngState rng) {
    float alpha = max(mat.metalRoughness * mat.metalRoughness, EPS_HIGH);
    float3 h;
    float3 wi = ggxScatter(mat, hit, wo, alpha, h, rng);
    GgxTerms t = computeGgxTerms(alpha, hit, wo, wi, h);
    float3 F = schlickAlbedo(t.VoH, mat.albedo);

    BSDFSample bsdf;
    bsdf.wi      = wi;
    bsdf.weight  = F * t.G * t.VoH / (t.cosWo * t.NoH);
    bsdf.pdf     = ggxPDF(t);
    bsdf.isDelta = false;
    bsdf.medium.isDielectric = false;
    bsdf.medium.isVolume = false;
    return bsdf;
}
