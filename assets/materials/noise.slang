import "script.slang";

struct Noise : IMaterial {
    [Min(0)] [Animatable] int seed;
    [Min(0)] int type;
    [Min(0)] float scale = 1;

    IBsdf evaluate(inout Hit hit, float3 wo, inout RngState rng) {
        float2 uv = hit.uv;
        RngState localRng = initRngState(uint(seed));
        float2 local = uv / scale;

        float r;
        switch (type) {
        case 0: {
            VoronoiState state = voronoiNoise(local, 1, localRng);
            offsetRngState(localRng, initRngState(state.cellIndex, 0));
            r = rand(localRng);
            break;
        } case 1: {
            VoronoiState state = voronoiNoise(local, 1, localRng);
            r = state.distToCenter;
            break;
        } case 2: {
            NoiseState2D state = valueNoise(local*2, localRng);
            r = state.value;
            break;
        } case 3: {
            NoiseState2D state = perlinNoise(local, localRng);
            r = state.value;
            break;
        } case 4: {
            NoiseState2D state = simplexNoise(local*0.8, localRng);
            r = state.value;
            break;
        } default: r = -1; break;
        }

        if (r < 0) {
            return Diffuse(float3(1, 0, 1));
        } else {
            float3 color = float3(viridis(r));
            return Diffuse(color);
        }
    }
}
