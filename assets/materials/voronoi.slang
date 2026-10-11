#material: version(1)

#param int seed: min(0)
#param float scale = 1.0: min(0)
#param float randomness = 1.0: min(0), max(1)
#param float3 borderColor: color
#param float borderSize = 0.01: min(0), max(0)


void main() {
    float2 local = uv / scale;

    rng = initRngState(uint(seed));
    VoronoiState state = voronoiNoise(local, randomness, rng);

    int2 idx = state.cellIndex;
    offsetRngState(rng, initRngState(idx, 0));
    float3 albedo = hsv2rgb(HSV(rand(rng)*360, lerp(0.5, 0.85, rand(rng)), lerp(0.8, 1.0, rand(rng))));
    // float3 albedo = float3(rand(rng));

    if (state.distToBorder < borderSize) albedo = borderColor;
    mat = Diffuse(albedo);
}

