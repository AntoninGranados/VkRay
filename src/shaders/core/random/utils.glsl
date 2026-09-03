#ifndef RANDOM_UTILS_GLSL
#define RANDOM_UTILS_GLSL

#include "basics.glsl"
#include "perlin.glsl"

const mat3 NOISE_ROTATION = mat3(
    0.00,  0.80,  0.60,
    -0.80,  0.36, -0.48,
    -0.60, -0.48,  0.64
);

const float FBM_SIGMA = 0.115;

float fractalNoise(vec3 p, int octaves, float lacunarity, float gain) {
    RngState rng = RngState(0);

    float amplitude = 1.0;
    float maxAmplitude = 0.0;
    float sum = 0.0;
    vec3 q = NOISE_ROTATION * p;
    for (int i = 0; i < octaves; i++) {
        sum += amplitude * perlinNoise(q, rng) * 2 - 1;
        maxAmplitude += amplitude;
        amplitude *= gain;
        q = NOISE_ROTATION * (q * lacunarity);
    }
    return sum / (maxAmplitude * FBM_SIGMA);
}

float turbulence(vec3 p, int octaves, float lacunarity, float gain) {
    RngState rng = RngState(0);

    float amplitude = 1.0;
    float maxAmplitude = 0.0;
    float sum = 0.0;
    vec3 q = NOISE_ROTATION * p;
    for (int i = 0; i < octaves; i++) {
        sum += amplitude * abs(perlinNoise(q, rng) * 2 - 1);
        maxAmplitude += amplitude;
        amplitude *= gain;
        q = NOISE_ROTATION * (q * lacunarity);
    }
    return sum / (maxAmplitude * 0.16);
}

#endif
