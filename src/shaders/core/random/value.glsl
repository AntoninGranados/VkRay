#ifndef RANDOM_VALUE_GLSL
#define RANDOM_VALUE_GLSL

#include "basics.glsl"

NoiseState2D valueNoise(vec2 p, RngState rng) {
    hashRngState(rng);

    vec2 f = fract(p);
    ivec2 p0 = ivec2(floor(p));

    float u = f.x;
    float v = f.y;

    float n[2], nx[2];

    RngState localRng;
    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 2; x++) {
            localRng = initRngState(p0 + ivec2(x, y), 0);
            offsetRngState(localRng, rng);

            n[x] = rand(localRng);
        }
        nx[y] = mix(n[0], n[1], u);
    }

    float r = mix(nx[0], nx[1], v);
    return NoiseState2D(r, vec2(0));
}

NoiseState3D valueNoise(vec3 p, RngState rng) {
    hashRngState(rng);

    vec3 f = fract(p);
    ivec3 p0 = ivec3(floor(p));

    float u = f.x;
    float v = f.y;
    float w = f.z;

    float n[2], nx[2], nxy[2];

    RngState localRng;
    for (int z = 0; z < 2; z++) {
        for (int y = 0; y < 2; y++) {
            for (int x = 0; x < 2; x++) {
                localRng = initRngState(p0 + ivec3(x, y, z), 0);
                offsetRngState(localRng, rng);

                n[x] = rand(localRng);
            }
            nx[y] = mix(n[0], n[1], u);
        }
        nxy[z] = mix(nx[0], nx[1], v);
    }

    float r = mix(nxy[0], nxy[1], w);
    return NoiseState3D(r, vec3(0));
}

#endif
