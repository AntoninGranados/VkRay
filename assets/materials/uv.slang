// --------------- INPUTS ---------------
// SurfacePoint surface (object, position, normal, uv, frontFace, vertexColor)
// float3 wo
// RngState rng

// --------------- OUTPUTS ---------------
// surface.normal
// IBsdf (return value)

import "script.slang";

struct Uv : IMaterial {
    IBsdf evaluate(inout SurfacePoint surface, float3 wo, inout RngState rng) {
        float2 uv = surface.uv;
        return Diffuse(clamp(float3(uv.x, uv.y, 0.0), float3(0), float3(1)));
    }
}
