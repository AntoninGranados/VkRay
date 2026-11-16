import "../attributes.slang";
import "../scene.slang";
import "../utils.slang";
import "../random/utils.slang";

import "material_utils.slang";
import "bsdf.slang";

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

[Medium]
struct VolumeBsdf : IBsdf {
    [Range(0, 1)] [Color] [Animatable] float3 albedo = float3(0.8, 0.8, 0.8);
    [Min(0)] [Animatable] float density = 1.0;
    [Range(-1, 1)] [Animatable] float anisotropic = 0.0;
    [Min(0)] int priority = 0;

    override bool isMediumBoundary() {
        return true;
    }

    MediumEntry mediumEntry(Object object) {
        return MediumEntry(mediumObjectKey(object), priority, activeIor(-1), albedo, density, 1.0, anisotropic);
    }

    BSDFEval eval(SurfacePoint surface, float3 wo, float3 wi) {
        return BSDFEval(
            float3(1.0),
            0.0
        );
    }

    BSDFSample sample(SurfacePoint surface, float3 wo, inout RngState rng) {
        crossInterface(mediumEntry(surface.object), surface);

        BSDFSample bsdf;
        bsdf.wi      = -wo;
        bsdf.weight  = float3(1.0);
        bsdf.pdf     = 0.0;
        bsdf.isDelta = true;
        bsdf.medium.isDielectric  = false;
        bsdf.medium.isVolume      = surface.frontFace;
        bsdf.medium.absorption    = albedo;
        bsdf.medium.density       = density;
        bsdf.medium.scatterAlbedo = 1.0;
        bsdf.medium.anisotropic   = anisotropic;
        return bsdf;
    }

    BsdfProperties properties() {
        return BsdfProperties(mat_Volume, albedo, 1.0, float3(0.0), true);
    }

    override Optional<MediumEntry> medium(Object object) {
        return mediumEntry(object);
    }
}
