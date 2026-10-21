import "script.slang";

struct Voronoi : IMaterial {
    [Min(0)] int seed;
    [Min(0)] float scale = 1.0;
    [Range(0, 1)] float randomness = 1.0;
    [Color] float3 borderColor;
    [Range(0, 0)] float borderSize = 0.01;

    IBsdf evaluate(inout Hit hit, float3 wo, inout RngState rng) {
        float2 uv = hit.uv;
        float2 local = uv / scale;

        rng = initRngState(uint(seed));
        VoronoiState state = voronoiNoise(local, randomness, rng);

        int2 idx = state.cellIndex;
        offsetRngState(rng, initRngState(idx, 0));
        float3 albedo = hsv2rgb(HSV(rand(rng)*360, lerp(0.5, 0.85, rand(rng)), lerp(0.8, 1.0, rand(rng))));
        // float3 albedo = float3(rand(rng));

        if (state.distToBorder < borderSize) albedo = borderColor;
        return Diffuse(albedo);
    }
}
