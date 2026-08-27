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

uint initSeed(uvec3 pos, uint frame) {
    uint v = pos.x + pos.y * 4096u + pos.z * 4096u * 4096u + frame * 1315423911u;
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

float fade(float t) {
    return 6 * pow(t, 5) - 15 * pow(t, 4) + 10 * pow(t, 3);
}

float perlinNoise(vec3 p) {
    vec3 cell = p;
    vec3 f = fract(cell);
    ivec3 p0 = ivec3(floor(cell));

    uint s;
    s = initSeed(p0 + ivec3(0, 0, 0), 0); vec3 grad000 = randomInSphere(s);
    s = initSeed(p0 + ivec3(1, 0, 0), 0); vec3 grad100 = randomInSphere(s);
    s = initSeed(p0 + ivec3(0, 1, 0), 0); vec3 grad010 = randomInSphere(s);
    s = initSeed(p0 + ivec3(1, 1, 0), 0); vec3 grad110 = randomInSphere(s);
    s = initSeed(p0 + ivec3(0, 0, 1), 0); vec3 grad001 = randomInSphere(s);
    s = initSeed(p0 + ivec3(1, 0, 1), 0); vec3 grad101 = randomInSphere(s);
    s = initSeed(p0 + ivec3(0, 1, 1), 0); vec3 grad011 = randomInSphere(s);
    s = initSeed(p0 + ivec3(1, 1, 1), 0); vec3 grad111 = randomInSphere(s);

    float n000 = dot(grad000, f - vec3(0, 0, 0));
    float n100 = dot(grad100, f - vec3(1, 0, 0));
    float n010 = dot(grad010, f - vec3(0, 1, 0));
    float n110 = dot(grad110, f - vec3(1, 1, 0));
    float n001 = dot(grad001, f - vec3(0, 0, 1));
    float n101 = dot(grad101, f - vec3(1, 0, 1));
    float n011 = dot(grad011, f - vec3(0, 1, 1));
    float n111 = dot(grad111, f - vec3(1, 1, 1));

    float u = fade(f.x); float v = fade(f.y); float w = fade(f.z);

    float nx00 = mix(n000, n100, u);
    float nx10 = mix(n010, n110, u);
    float nx01 = mix(n001, n101, u);
    float nx11 = mix(n011, n111, u);

    float nxy0 = mix(nx00, nx10, v);
    float nxy1 = mix(nx01, nx11, v);

    return mix(nxy0, nxy1, w);
}

const mat3 NOISE_ROTATION = mat3(
    0.00,  0.80,  0.60,
    -0.80,  0.36, -0.48,
    -0.60, -0.48,  0.64
);

const float FBM_SIGMA = 0.115;

float fractalNoise(vec3 p, int octaves, float lacunarity, float gain) {
    float amplitude = 1.0;
    float maxAmplitude = 0.0;
    float sum = 0.0;
    vec3 q = NOISE_ROTATION * p;
    for (int i = 0; i < octaves; i++) {
        sum += amplitude * perlinNoise(q);
        maxAmplitude += amplitude;
        amplitude *= gain;
        q = NOISE_ROTATION * (q * lacunarity);
    }
    return sum / (maxAmplitude * FBM_SIGMA);
}

float turbulence(vec3 p, int octaves, float lacunarity, float gain) {
    float amplitude = 1.0;
    float maxAmplitude = 0.0;
    float sum = 0.0;
    vec3 q = NOISE_ROTATION * p;
    for (int i = 0; i < octaves; i++) {
        sum += amplitude * abs(perlinNoise(q));
        maxAmplitude += amplitude;
        amplitude *= gain;
        q = NOISE_ROTATION * (q * lacunarity);
    }
    return sum / (maxAmplitude * 0.16);
}

#endif
