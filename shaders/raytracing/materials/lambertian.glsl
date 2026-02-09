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

float lambertianPDF(in Material mat, in Hit hit, in vec3 wo, in vec3 wi) {
    return max(dot(hit.normal, wi), 0.0) / PI;
}

vec3 lambertianF(in Material mat, in Hit hit, in vec3 wo, in vec3 wi) {
    return mat.albedo / PI;
}

void sampleLambertianBSDF(in Material mat, in Hit hit, in vec3 wo, out SampleResult result, inout uint seed) {
    result.wi = cosineScatter(mat, hit.normal, wo, seed);

    result.f = lambertianF(mat, hit, wo, result.wi);
    result.pdf = lambertianPDF(mat, hit, wo, result.wi);
    result.isDelta = false;
    result.isTransmission = false;
}

#endif // LAMBERTIAN_GLSL