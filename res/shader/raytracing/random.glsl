#ifndef RANDOM_GLSL
#define RANDOM_GLSL

vec3 initSeed(vec2 pos, float time) {
    return vec3(
        fract(sin(dot(pos, vec2(12.9898,78.233))) * 43758.5453 + time),
        fract(sin(dot(pos, vec2(93.9898,67.345))) * 43758.5453 + time),
        fract(sin(dot(pos, vec2(56.1234,12.345))) * 43758.5453 + time)
    );
}

float hash13(vec3 p3) {
    p3  = fract(p3 * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float rand(inout vec3 seed) {
    float r = hash13(seed);
    seed += vec3(1.0, 1.0, 1.0);
    return r;
}

vec3 randomInSphere(inout vec3 seed) {
    float z  = 1.0 - 2.0 * rand(seed);
    float r  = sqrt(max(0.0, 1.0 - z*z));
    float phi = 6.2831853 * rand(seed);
    float x = r * cos(phi);
    float y = r * sin(phi);
    return vec3(x, y, z);
}

vec3 randomInHemisphere(inout vec3 seed, vec3 normal) {
    vec3 v = randomInSphere(seed);
    return dot(v, normal) < 0.0 ? -v : v;
}

vec2 randomInDisk(inout vec3 seed) {
    float r  = sqrt(rand(seed));
    float theta = 6.2831853 * rand(seed);
    float x = r * cos(theta);
    float y = r * sin(theta);
    return vec2(x, y);
}

#endif
