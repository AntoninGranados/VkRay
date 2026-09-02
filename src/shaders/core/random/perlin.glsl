#ifndef RANDOM_PERLIN_GLSL
#define RANDOM_PERLIN_GLSL

#include "basics.glsl"

float fade(float t) {
    return 6 * pow(t, 5) - 15 * pow(t, 4) + 10 * pow(t, 3);
}

float perlinNoise(in vec2 p, inout RngState rng) {
    hashRngState(rng);

    vec2 cell = p;
    vec2 f = fract(cell);
    ivec2 p0 = ivec2(floor(cell));
    
    float u = fade(f.x);
    float v = fade(f.y);
    
    float n[2][2];
    float nx[2];

    RngState localRng;
    vec2 grad;
    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 2; x++) {
            localRng = initRngState(ivec3(p0 + ivec2(x, y), 0), 0);
            offsetRngState(localRng, rng);

            grad = randomInDisk(localRng);
            n[x][y] = dot(grad, f - vec2(x, y));
        }
        nx[y] = mix(n[0][y], n[1][y], u);
    }

    float r = mix(nx[0], nx[1], v);
    return r * 0.5 + 0.5;
}

float perlinNoise(in vec3 p, inout RngState rng) {
    hashRngState(rng);

    vec3 cell = p;
    vec3 f = fract(cell);
    ivec3 p0 = ivec3(floor(cell));
    
    float u = fade(f.x);
    float v = fade(f.y);
    float w = fade(f.z);

    float n[2][2][2];
    float nx[2][2];
    float nxy[2];

    RngState localRng;
    vec3 grad;
    for (int z = 0; z < 2; z++) {
        for (int y = 0; y < 2; y++) {
            for (int x = 0; x < 2; x++) {
                localRng = initRngState(p0 + ivec3(x, y, z), 0);
                offsetRngState(localRng, rng);

                grad = randomInSphere(localRng);
                n[x][y][z] = dot(grad, f - vec3(x, y, z));
            }
            nx[y][z] = mix(n[0][y][z], n[1][y][z], u);
        }
        nxy[z] = mix(nx[0][z], nx[1][z], v);
    }

    float r = mix(nxy[0], nxy[1], w);
    return r * 0.5 + 0.5;
}

#endif
