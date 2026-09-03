#ifndef DIFFUSE_GLSL
#define DIFFUSE_GLSL

#include "../utils.glsl"
#include "../random/utils.glsl"

#include "material_utils.glsl"

// Lambertian BSDF: perfectly diffuse with cosine-weighted importance sampling.

vec3 cosineScatter(in ResolvedMaterial mat, in vec3 normal, in vec3 wo, inout RngState rng) {
    vec3 dir = normal + normalize(randomInSphere(rng));
    if (length(dir) < EPS) return normal;
    return normalize(dir);
}

BSDFEval evalDiffuseBSDF(in ResolvedMaterial mat, in Hit hit, in vec3 wo, in vec3 wi) {
    return BSDFEval(
        albedo(mat) / PI,
        max(dot(hit.normal, wi), 0.0) / PI
    );
}

BSDFSample sampleDiffuseBSDF(in ResolvedMaterial mat, in Hit hit, in vec3 wo, inout RngState rng) {
    vec3 wi = cosineScatter(mat, hit.normal, wo, rng);
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

#endif // DIFFUSE_GLSL
