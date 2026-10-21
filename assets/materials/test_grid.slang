import "script.slang";

struct TestGrid : IMaterial {
    [Color] [Animatable] float3 albedo = float3(1.0);
    [Min(0)] [Animatable] float scale = 1.0;
    [Min(0)] [Animatable] float feather = 0.02;
    [Range(0, 1)] [Animatable] float darkening = 0.8;

    IBsdf evaluate(inout Hit hit, float3 wo, inout RngState rng) {
        float2 uv = hit.uv;
        const float small_feather = feather * 0.5;

        if (abs(uv.x - round(uv.x / scale) * scale) < feather || abs(uv.y - round(uv.y / scale) * scale) < feather) {
            return Diffuse(float3(0.0));
        } else if (int(round(uv.x / scale + 0.5) + round(uv.y / scale + 0.5) + 1) % 2 == 0) {
            if (abs(uv.x - round(uv.x / scale * 2) * scale / 2) < small_feather || abs(uv.y - round(uv.y / scale * 2) * scale / 2) < small_feather) {
                return Diffuse(float3(0.0));
            } else {
                return Glossy(albedo, 0.1, 2.0);
            }
        } else {
            return Glossy(albedo * darkening, 0.1, 2.0);
        }
    }
}
