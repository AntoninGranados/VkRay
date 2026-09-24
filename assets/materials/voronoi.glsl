#material: version(1)

#param int seed: min(0)
#param float scale = 1.0: min(0)
#param float randomness = 1.0: min(0), max(1)
#param vec3 borderColor: color
#param float borderSize = 0.01: min(0), max(0)


void main() {
    vec2 local = uv / scale;

    rng = RngState(seed);
    VoronoiState state = voronoiNoise(local, randomness, rng);

    ivec2 idx = state.cellIndex;
    offsetRngState(rng, initRngState(idx, 0));
    // vec3 albedo = hsv2rgb(vec3(rand(rng)*360, mix(0.5, 0.85, rand(rng)), mix(0.8, 1.0, rand(rng))));
    vec3 albedo = vec3(rand(rng));

    if (state.distToBorder < borderSize) albedo = borderColor;
    mat = Diffuse(albedo);
}

