#ifndef RANDOM_SIMPLEX_GLSL
#define RANDOM_SIMPLEX_GLSL

#include "basics.glsl"

#define SIMPLEX_R 0.5
NoiseState2D simplexNoise(vec2 p, RngState rng) {
    const float N = 2;
    const float F = (sqrt(N + 1) - 1) / N;
    const float G = (1 - 1 / sqrt(N + 1)) / N;

    vec2 ps = p + vec2(p.x + p.y) * F;
    vec2 idx = floor(ps);
    vec2 local = fract(ps);

    vec2 P0 = idx - vec2(idx.x + idx.y) * G;
    vec2 p0 = p - P0;

    vec2 corners[3];
    corners[0] = vec2(0);
    if (p0.x > p0.y) corners[1] = vec2(1, 0);
    else corners[1] = vec2(0, 1);
    corners[2] = vec2(1);

    float n[3];
    RngState localRng;
    vec2 offset, grad;
    for (int i = 0; i < 3; i++) {
        offset = p0 - (corners[i] - vec2(corners[i].x + corners[i].y) * G);

        localRng = initRngState(ivec2(idx + corners[i]), 0);
        offsetRngState(localRng, rng);
        grad = randomOnCircle(localRng);

        n[i] = dot(offset, grad);
        n[i] *= pow(max(0, SIMPLEX_R - dot(offset, offset)), 4);
    }

    float r = (n[0] + n[1] + n[2]) * 70;
    return NoiseState2D(r * 0.5 + 0.5, vec2(0));
}

// TODO: implement the 3D version

#endif
