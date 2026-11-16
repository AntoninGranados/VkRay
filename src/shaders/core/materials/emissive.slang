import "../attributes.slang";
import "../scene.slang";
import "../utils.slang";
import "../random/utils.slang";

import "material_utils.slang";
import "bsdf.slang";

[Emissive]
struct EmissiveBsdf : IBsdf {
    [Range(0, 1)] [Color] [Animatable] float3 albedo = float3(1.0, 1.0, 1.0);
    [Min(0)] [Animatable] float emissionStrength = 1.0;

    BSDFEval eval(SurfacePoint surface, float3 wo, float3 wi) {
        return BSDFEval(float3(0.0), 0.0);
    }

    BSDFSample sample(SurfacePoint surface, float3 wo, inout RngState rng) {
        return BSDFSample(float3(0.0), surface.normal, 0.0, false, NO_MEDIUM);
    }

    BsdfProperties properties() {
        return BsdfProperties(mat_Emissive, albedo, 1.0, albedo * emissionStrength);
    }
}
