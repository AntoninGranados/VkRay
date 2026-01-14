#ifndef RANDOM_GLSL
#define RANDOM_GLSL

uint pcg_hash(uint v) {
    v = v * 747796405u + 2891336453u;
    uint word = ((v >> ((v >> 28u) + 4u)) ^ v) * 277803737u;
    return (word >> 22u) ^ word;
}

uint initSeed(uvec2 pos, uint frame) {
    uint v = pos.x + pos.y * 4096u + frame * 1315423911u;
    return pcg_hash(v);
}

float rand(inout uint seed) {
    seed = pcg_hash(seed);
    return float(seed) * (1.0 / 4294967296.0);
}

vec3 randomInSphere(inout uint seed) {
    float z  = 1.0 - 2.0 * rand(seed);
    float r  = sqrt(max(0.0, 1.0 - z*z));
    float phi = 6.2831853 * rand(seed);
    float x = r * cos(phi);
    float y = r * sin(phi);
    return vec3(x, y, z);
}

vec3 randomInHemisphere(inout uint seed, vec3 normal) {
    vec3 v = randomInSphere(seed);
    return dot(v, normal) < 0.0 ? -v : v;
}

vec2 randomInDisk(inout uint seed) {
    float r  = sqrt(rand(seed));
    float theta = 6.2831853 * rand(seed);
    float x = r * cos(theta);
    float y = r * sin(theta);
    return vec2(x, y);
}

#endif
