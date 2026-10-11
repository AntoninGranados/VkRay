import "basics.slang";

NoiseState2D valueNoise(float2 p, RngState rng) {
    hashRngState(rng);

    float2 f = frac(p);
    int2 p0 = int2(floor(p));

    float u = f.x;
    float v = f.y;

    float n[2], nx[2];

    RngState localRng;
    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 2; x++) {
            localRng = initRngState(p0 + int2(x, y), 0);
            offsetRngState(localRng, rng);

            n[x] = rand(localRng);
        }
        nx[y] = lerp(n[0], n[1], u);
    }

    float r = lerp(nx[0], nx[1], v);
    return NoiseState2D(r, float2(0));
}

NoiseState3D valueNoise(float3 p, RngState rng) {
    hashRngState(rng);

    float3 f = frac(p);
    int3 p0 = int3(floor(p));

    float u = f.x;
    float v = f.y;
    float w = f.z;

    float n[2], nx[2], nxy[2];

    RngState localRng;
    for (int z = 0; z < 2; z++) {
        for (int y = 0; y < 2; y++) {
            for (int x = 0; x < 2; x++) {
                localRng = initRngState(p0 + int3(x, y, z), 0);
                offsetRngState(localRng, rng);

                n[x] = rand(localRng);
            }
            nx[y] = lerp(n[0], n[1], u);
        }
        nxy[z] = lerp(nx[0], nx[1], v);
    }

    float r = lerp(nxy[0], nxy[1], w);
    return NoiseState3D(r, float3(0));
}
