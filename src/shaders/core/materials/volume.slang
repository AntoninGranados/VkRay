import "../utils.slang";
import "../random/utils.slang";

import "material_utils.slang";

// Henyey–Greenstein phase function (https://en.wikipedia.org/wiki/Henyey–Greenstein_phase_function)
float phaseFunctionHG(float g, in float3 wi, in float3 wo) {
    float g_sq = g * g;
    float denom = 1.0f + g_sq - 2.0f * g * dot(wi, wo);
    denom = pow(denom, 1.5f);
    return (1.0f - g_sq) / denom / PI * 0.25f;
}

float3 sampleHG(float g, float3 wi, inout RngState rng) {
    float2 xi = float2(rand(rng), rand(rng));

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
    float3 up = abs(wi.z) < 0.999 ? float3(0,0,1) : float3(1,0,0);
    float3 tangent   = normalize(cross(up, wi));
    float3 bitangent = cross(wi, tangent);

    // Return new direction in world space
    return normalize(
        sin_theta * cos(phi) * tangent +
        sin_theta * sin(phi) * bitangent +
        cos_theta * wi
    );
}

BSDFEval evalVolumeBSDF(in ResolvedMaterial mat, in Hit hit, in float3 wo, in float3 wi) {
    return BSDFEval(
        float3(1.0),
        0.0
    );
}

BSDFSample sampleVolumeBSDF(in ResolvedMaterial mat, in Hit hit, in float3 wo, inout RngState rng) {
    crossInterface(mat, hit);

    BSDFSample bsdf;
    bsdf.wi      = -wo;
    bsdf.weight  = float3(1.0);
    bsdf.pdf     = 0.0;
    bsdf.isDelta = true;
    bsdf.medium.isDielectric  = false;
    bsdf.medium.isVolume      = hit.frontFace;
    bsdf.medium.absorption    = mat.albedo;
    bsdf.medium.density       = mat.volumeDensity;
    bsdf.medium.scatterAlbedo = 1.0;
    bsdf.medium.anisotropic   = mat.volumeAnisotropic;
    return bsdf;
}
