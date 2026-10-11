// --------------- INPUTS ---------------
// float3 pos
// float2 uv
// float3 normal
// float3 wo
// RngState rng

// --------------- OUTPUTS ---------------
// float3 new_normal = normal
// Material mat

#material: version(1)

void main() {
    mat = Diffuse(clamp(float3(uv.x, uv.y, 0.0), float3(0), float3(1)));
}
