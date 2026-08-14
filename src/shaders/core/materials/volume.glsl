#ifndef VOLUME_GLSL
#define VOLUME_GLSL

#include "../utils.glsl"
#include "../random.glsl"

#include "material_utils.glsl"

// Henyey–Greenstein phase function (https://en.wikipedia.org/wiki/Henyey–Greenstein_phase_function)
float phaseFunctionHG(float g, in vec3 wi, in vec3 wo) {
    float g_sq = g * g;
    float denom = 1.0f + g_sq - 2.0f * g * dot(wi, wo);
    denom = pow(denom, 1.5f);
    return (1.0f - g_sq) / denom / PI * 0.25f;
}

vec3 sampleHG(float g, vec3 wi, inout uint seed) {
    vec2 xi = vec2(rand(seed), rand(seed));

    // Sample cos_theta
    float cos_theta;
    if (abs(g) < 1e-4) {
        cos_theta = 1.0 - 2.0 * xi.x; // isotropic
    } else {
        float s = (1.0 - g*g) / (1.0 - g + 2.0*g*xi.x);
        cos_theta = (1.0 + g*g - s*s) / (2.0*g);
    }
    float sin_theta = sqrt(max(0.0, 1.0 - cos_theta*cos_theta));
    float phi = 2.0 * PI * xi.y;

    // Build orthonormal frame around wi
    vec3 up = abs(wi.z) < 0.999 ? vec3(0,0,1) : vec3(1,0,0);
    vec3 tangent   = normalize(cross(up, wi));
    vec3 bitangent = cross(wi, tangent);

    // Return new direction in world space
    return normalize(
        sin_theta * cos(phi) * tangent +
        sin_theta * sin(phi) * bitangent +
        cos_theta * wi
    );
}

BSDFEval evalVolumeBSDF(in Material mat, in Hit hit, in vec3 wo, in vec3 wi) {
    return BSDFEval(
        vec3(1.0),
        0.0
    );
}

BSDFSample sampleVolumeBSDF(in Material mat, in Hit hit, in vec3 wo, inout uint seed) {
    BSDFSample bsdf;
    bsdf.wi      = -wo;
    bsdf.weight  = vec3(1.0);
    bsdf.pdf     = 0.0;
    bsdf.isDelta = true;
    bsdf.medium.isDielectric  = false;
    bsdf.medium.isVolume      = hit.frontFace;
    bsdf.medium.absorption    = mat_albedo(mat);
    bsdf.medium.density       = mat_volume_density(mat);
    bsdf.medium.scatterAlbedo = 1.0;
    bsdf.medium.anisotropic   = mat_volume_anisotropic(mat);
    return bsdf;
}

#endif // VOLUME_GLSL