#material: version(1)

#param int seed: min(0), animatable
#param int type: min(0)
#param float scale = 1: min(0)

void main() {
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
        mat = Diffuse(float3(1, 0, 1));
    } else {
        float3 color = float3(viridis(r));
        mat = Diffuse(color);
    }
}
