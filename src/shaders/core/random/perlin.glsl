#ifndef RANDOM_PERLIN_GLSL
#define RANDOM_PERLIN_GLSL

#include "basics.glsl"

float fade(float t) {
    return 6 * pow(t, 5) - 15 * pow(t, 4) + 10 * pow(t, 3);
}

#define PERLIN_NOISE_2D_NORM 1.41421356237 // sqrt(2)
NoiseState2D perlinNoise(in vec2 p, inout RngState rng) {
    hashRngState(rng);

    vec2 f = fract(p);
    ivec2 p0 = ivec2(floor(p));

    float u = fade(f.x);
    float v = fade(f.y);

    float n[2], nx[2];

    RngState localRng;
    vec2 grad;
    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 2; x++) {
            localRng = initRngState(p0 + ivec2(x, y), 0);
            offsetRngState(localRng, rng);

            grad = randomOnCircle(localRng);
            n[x] = dot(grad, f - vec2(x, y));
        }
        nx[y] = mix(n[0], n[1], u);
    }

    float r = mix(nx[0], nx[1], v);
    r /= PERLIN_NOISE_2D_NORM;
    return NoiseState2D(r * 0.5 + 0.5, vec2(0));
}

// WARN: Nor normalized to be in [0,1] (no closed form value like in 2D ?)
// TODO: Find a normalization constant
NoiseState3D perlinNoise(in vec3 p, inout RngState rng) {
    hashRngState(rng);

    vec3 f = fract(p);
    ivec3 p0 = ivec3(floor(p));

    float u = fade(f.x);
    float v = fade(f.y);
    float w = fade(f.z);

    float n[2], nx[2], nxy[2];

    RngState localRng;
    vec3 grad;
    for (int z = 0; z < 2; z++) {
        for (int y = 0; y < 2; y++) {
            for (int x = 0; x < 2; x++) {
                localRng = initRngState(p0 + ivec3(x, y, z), 0);
                offsetRngState(localRng, rng);

                grad = randomOnSphere(localRng);
                n[x] = dot(grad, f - vec3(x, y, z));
            }
            nx[y] = mix(n[0], n[1], u);
        }
        nxy[z] = mix(nx[0], nx[1], v);
    }

    float r = mix(nxy[0], nxy[1], w);
    return NoiseState3D(r * 0.5 + 0.5, vec3(0));
}

#endif
