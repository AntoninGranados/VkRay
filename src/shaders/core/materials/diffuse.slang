import "../utils.slang";
import "../random/utils.slang";

import "material_utils.slang";

// Lambertian BSDF: perfectly diffuse with cosine-weighted importance sampling.

float3 cosineScatter(in ResolvedMaterial mat, in float3 normal, in float3 wo, inout RngState rng) {
    float3 dir = normal + randomOnSphere(rng);
    if (length(dir) < EPS) return normal;
    return normalize(dir);
}

BSDFEval evalDiffuseBSDF(in ResolvedMaterial mat, in Hit hit, in float3 wo, in float3 wi) {
    return BSDFEval(
        mat.albedo / PI,
        max(dot(hit.normal, wi), 0.0) / PI
    );
}

BSDFSample sampleDiffuseBSDF(in ResolvedMaterial mat, in Hit hit, in float3 wo, inout RngState rng) {
    float3 wi = cosineScatter(mat, hit.normal, wo, rng);
    BSDFEval eval = evalDiffuseBSDF(mat, hit, wo, wi);

    BSDFSample bsdf;
    float cosB = abs(dot(hit.normal, wi));
    bsdf.wi      = wi;
    bsdf.weight  = eval.f * cosB / eval.pdf;
    bsdf.pdf     = eval.pdf;
    bsdf.isDelta = false;
    bsdf.medium.isDielectric = false;
    bsdf.medium.isVolume = false;
    return bsdf;
}
