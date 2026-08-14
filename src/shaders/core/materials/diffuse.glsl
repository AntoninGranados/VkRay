#ifndef DIFFUSE_GLSL
#define DIFFUSE_GLSL

#include "../utils.glsl"
#include "../random.glsl"

#include "material_utils.glsl"

// Lambertian BSDF: perfectly diffuse with cosine-weighted importance sampling.

vec3 cosineScatter(in Material mat, in vec3 normal, in vec3 wo, inout uint seed) {
    vec3 dir = normal + normalize(randomInSphere(seed));
    if (length(dir) < EPS) return normal;
    return normalize(dir);
}

BSDFEval evalDiffuseBSDF(in Material mat, in Hit hit, in vec3 wo, in vec3 wi) {
    return BSDFEval(
        mat_albedo(mat) / PI,
        max(dot(hit.normal, wi), 0.0) / PI
    );
}

BSDFSample sampleDiffuseBSDF(in Material mat, in Hit hit, in vec3 wo, inout uint seed) {
    vec3 wi = cosineScatter(mat, hit.normal, wo, seed);
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
