import "script.slang";

struct PointGrid : IMaterial {
    [Min(0)] float scale = 1.0;
    [Color] float3 albedo = float3(1.0);
    [Min(0)] float radius = 0.08;

    IBsdf evaluate(inout Hit hit, float3 wo, inout RngState rng) {
        float2 uv = hit.uv;
        float x = abs(uv.x - round(uv.x / scale) * scale);
        float y = abs(uv.y - round(uv.y / scale) * scale);
        if (x * x + y * y < radius * radius) {
            return Glossy(albedo, 0.1, 2.0);
        } else {
            return Diffuse(float3(0.02));
        }
    }
}
