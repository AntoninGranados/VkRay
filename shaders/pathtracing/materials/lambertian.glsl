#ifndef LAMBERTIAN_GLSL
#define LAMBERTIAN_GLSL

#include "../utils.glsl"
#include "../random.glsl"

#include "material_utils.glsl"

vec3 cosineScatter(in Material mat, in vec3 normal, in vec3 wo, inout uint seed) {
    vec3 dir = normal + normalize(randomInSphere(seed));
    if (length(dir) < EPS) return normal;
    return normalize(dir);
}

BSDFEval evalLambertianBSDF(in Material mat, in Hit hit, in vec3 wo, in vec3 wi) {
    return BSDFEval(
        mat.albedo / PI,
        max(dot(hit.normal, wi), 0.0) / PI
    );
}

BSDFSample sampleLambertianBSDF(in Material mat, in Hit hit, in vec3 wo, inout uint seed) {
    vec3 wi = cosineScatter(mat, hit.normal, wo, seed);
    BSDFEval eval = evalLambertianBSDF(mat, hit, wo, wi);
    
    BSDFSample bsdf;
    float cosB = abs(dot(hit.normal, wi));
    bsdf.wi      = wi;
    bsdf.weight  = eval.f * cosB / eval.pdf;
    bsdf.pdf     = eval.pdf;
    bsdf.isDelta = false;

    return bsdf;
}

#endif // LAMBERTIAN_GLSL