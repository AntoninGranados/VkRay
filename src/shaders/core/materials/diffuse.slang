import "../attributes.slang";
import "../scene.slang";
import "../utils.slang";
import "../random/utils.slang";

import "material_utils.slang";
import "bsdf.slang";

// Lambertian BSDF: perfectly diffuse with cosine-weighted importance sampling.

float3 cosineScatter(in float3 normal, inout RngState rng) {
    float3 dir = normal + randomOnSphere(rng);
    if (length(dir) < EPS) return normal;
    return normalize(dir);
}

struct DiffuseBsdf : IBsdf {
    [Range(0, 1)] [Color] [Animatable] float3 albedo = float3(0.8, 0.8, 0.8);

    BSDFEval eval(SurfacePoint surface, float3 wo, float3 wi) {
        return BSDFEval(
            albedo / PI,
            max(dot(surface.normal, wi), 0.0) / PI
        );
    }

    BSDFSample sample(SurfacePoint surface, float3 wo, inout RngState rng) {
        float3 wi = cosineScatter(surface.normal, rng);
        BSDFEval eval = this.eval(surface, wo, wi);

        float cosB = abs(dot(surface.normal, wi));
        return BSDFSample(eval.f * cosB / eval.pdf, wi, eval.pdf, false, NO_MEDIUM);
    }

    BsdfProperties properties() {
        return BsdfProperties(mat_Diffuse, albedo, 1.0);
    }
}
