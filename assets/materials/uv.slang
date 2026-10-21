// --------------- INPUTS ---------------
// Hit hit (p, uv, normal, vertexColor)
// float3 wo
// RngState rng

// --------------- OUTPUTS ---------------
// hit.normal
// IBsdf (return value)

import "script.slang";

struct Uv : IMaterial {
    IBsdf evaluate(inout Hit hit, float3 wo, inout RngState rng) {
        float2 uv = hit.uv;
        return Diffuse(clamp(float3(uv.x, uv.y, 0.0), float3(0), float3(1)));
    }
}
