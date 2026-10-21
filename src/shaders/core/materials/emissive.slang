import "../scene.slang";
import "../utils.slang";
import "../random/utils.slang";

import "material_utils.slang";
import "bsdf.slang";

struct EmissiveBsdf : IBsdf {
    float3 albedo;
    float strength;

    BSDFEval eval(Hit hit, float3 wo, float3 wi) {
        return BSDFEval(float3(0.0), 0.0);
    }

    BSDFSample sample(Hit hit, float3 wo, inout RngState rng) {
        return BSDFSample(float3(0.0), hit.normal, 0.0, false, NO_MEDIUM);
    }

    BsdfProperties properties() {
        return BsdfProperties(mat_Emissive, albedo, 1.0, albedo * strength);
    }
}

struct EmissiveMaterial : IMaterial {
    uint base;

    IBsdf evaluate(inout Hit hit, float3 wo, inout RngState rng) {
        EmissiveBsdf bsdf = loadParams<EmissiveBsdf>(base);
        bsdf.albedo *= hit.vertexColor;
        return bsdf;
    }
}
