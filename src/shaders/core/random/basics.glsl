#ifndef RANDOM_BASICS_GLSL
#define RANDOM_BASICS_GLSL

struct RngState {
    uint seed;
};

void offsetRngState(inout RngState rng, in RngState rngOffset) {
    rng.seed += rngOffset.seed;
}

uint pcgHash(uint v) {
    v = v * 747796405u + 2891336453u;
    uint word = ((v >> ((v >> 28u) + 4u)) ^ v) * 277803737u;
    return (word >> 22u) ^ word;
}

RngState initRngState(uvec2 pos, uint frame) {
    uint v = pos.x + pos.y * 4096u + frame * 1315423911u;
    return RngState(pcgHash(v));
}

RngState initRngState(uvec3 pos, uint frame) {
    uint v = pos.x + pos.y * 4096u + pos.z * 4096u * 4096u + frame * 1315423911u;
    return RngState(pcgHash(v));
}

void hashRngState(inout RngState rng) {
    rng.seed = pcgHash(rng.seed);    
}

#define UINT_MAX uint(-1)
float rand(inout RngState rng) {
    hashRngState(rng);
    return float(rng.seed) / float(UINT_MAX);
}

vec2 rand2(inout RngState rng) {
    return vec2(rand(rng), rand(rng));
}

vec3 rand3(inout RngState rng) {
    return vec3(rand2(rng), rand(rng));
}

vec4 rand4(inout RngState rng) {
    return vec4(rand3(rng), rand(rng));
}

vec3 randomInSphere(inout RngState rng) {
    float z  = 1.0 - 2.0 * rand(rng);
    float r  = sqrt(max(0.0, 1.0 - z*z));
    float phi = 6.2831853 * rand(rng);
    float x = r * cos(phi);
    float y = r * sin(phi);
    return vec3(x, y, z);
}

vec3 randomInHemisphere(inout RngState rng, vec3 normal) {
    vec3 v = randomInSphere(rng);
    return dot(v, normal) < 0.0 ? -v : v;
}

vec2 randomInDisk(inout RngState rng) {
    float r  = sqrt(rand(rng));
    float theta = 6.2831853 * rand(rng);
    float x = r * cos(theta);
    float y = r * sin(theta);
    return vec2(x, y);
}

#endif
