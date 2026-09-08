#material: version(1)

#param int seed: min(0), animatable
#param int type: min(0)
#param float scale = 1: min(0)

void main() {
    RngState rng = RngState(seed);
    vec2 local = uv / scale;

    float r;
    switch (type) {
    case 0: {
        VoronoiState state = voronoiNoise(local, 1, rng);
        offsetRngState(rng, initRngState(state.cellIndex, 0));
        r = rand(rng);
        break;
    } case 1: {
        VoronoiState state = voronoiNoise(local, 1, rng);
        r = state.distToCenter;
        break;
    } case 2: {
        NoiseState2D state = valueNoise(local*2, rng);
        r = state.value;
        break;
    } case 3: {
        NoiseState2D state = perlinNoise(local, rng);
        r = state.value;
        break;
    } case 4: {
        NoiseState2D state = simplexNoise(local*0.8, rng);
        r = state.value;
        break;
    } default: r = -1; break;
    }

    if (r < 0) {
        mat = Diffuse(vec3(1, 0, 1));
    } else {
        vec3 color = vec3(viridis(r));
        mat = Diffuse(color);
    }
}
