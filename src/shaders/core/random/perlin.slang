import "basics.slang";

float fade(float t) {
    return 6 * pow(t, 5) - 15 * pow(t, 4) + 10 * pow(t, 3);
}

static const float PERLIN_NOISE_2D_NORM = 1.41421356237; // sqrt(2)
NoiseState2D perlinNoise(in float2 p, inout RngState rng) {
    hashRngState(rng);

    float2 f = frac(p);
    int2 p0 = int2(floor(p));

    float u = fade(f.x);
    float v = fade(f.y);

    float n[2], nx[2];

    RngState localRng;
    float2 grad;
    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 2; x++) {
            localRng = initRngState(p0 + int2(x, y), 0);
            offsetRngState(localRng, rng);

            grad = randomOnCircle(localRng);
            n[x] = dot(grad, f - float2(x, y));
        }
        nx[y] = lerp(n[0], n[1], u);
    }

    float r = lerp(nx[0], nx[1], v);
    r *= PERLIN_NOISE_2D_NORM;
    return NoiseState2D(r * 0.5 + 0.5, float2(0));
}

static const float PERLIN_NOISE_3D_NORM = 2.0 / sqrt(3.0);
NoiseState3D perlinNoise(in float3 p, inout RngState rng) {
    hashRngState(rng);

    float3 f = frac(p);
    int3 p0 = int3(floor(p));

    float u = fade(f.x);
    float v = fade(f.y);
    float w = fade(f.z);

    float n[2], nx[2], nxy[2];

    RngState localRng;
    float3 grad;
    for (int z = 0; z < 2; z++) {
        for (int y = 0; y < 2; y++) {
            for (int x = 0; x < 2; x++) {
                localRng = initRngState(p0 + int3(x, y, z), 0);
                offsetRngState(localRng, rng);

                grad = randomOnSphere(localRng);
                n[x] = dot(grad, f - float3(x, y, z));
            }
            nx[y] = lerp(n[0], n[1], u);
        }
        nxy[z] = lerp(nx[0], nx[1], v);
    }

    float r = lerp(nxy[0], nxy[1], w);
    r *= PERLIN_NOISE_3D_NORM;
    return NoiseState3D(r * 0.5 + 0.5, float3(0));
}
