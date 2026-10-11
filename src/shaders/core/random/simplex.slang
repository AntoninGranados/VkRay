import "basics.slang";

static const float SIMPLEX_R = 0.5;
NoiseState2D simplexNoise(float2 p, RngState rng) {
    const float N = 2;
    const float F = (sqrt(N + 1) - 1) / N;
    const float G = (1 - 1 / sqrt(N + 1)) / N;

    float2 ps = p + float2(p.x + p.y) * F;
    float2 idx = floor(ps);
    float2 local = frac(ps);

    float2 P0 = idx - float2(idx.x + idx.y) * G;
    float2 p0 = p - P0;

    float2 corners[3];
    corners[0] = float2(0);
    if (p0.x > p0.y) corners[1] = float2(1, 0);
    else corners[1] = float2(0, 1);
    corners[2] = float2(1);

    float n[3];
    RngState localRng;
    float2 offset, grad;
    for (int i = 0; i < 3; i++) {
        offset = p0 - (corners[i] - float2(corners[i].x + corners[i].y) * G);

        localRng = initRngState(int2(idx + corners[i]), 0);
        offsetRngState(localRng, rng);
        grad = randomOnCircle(localRng);

        n[i] = dot(offset, grad);
        n[i] *= pow(max(0, SIMPLEX_R - dot(offset, offset)), 4);
    }

    float r = (n[0] + n[1] + n[2]) * 70;
    return NoiseState2D(r * 0.5 + 0.5, float2(0));
}

// TODO: implement the 3D version
