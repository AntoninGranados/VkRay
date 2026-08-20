#ifndef PROGRAMMABLE_GLSL
#define PROGRAMMABLE_GLSL

#include "../utils.glsl"
#include "../random.glsl"

#include "material_utils.glsl"
#include "diffuse.glsl"
#include "metal.glsl"
#include "glossy.glsl"

#define SCALE 1.0
#define FEATHER 0.02
#define SMALL_FEATHER 0.01

#define PROGRAMMABLE_SMOOTH_RANDOM
// #define PROGRAMMABLE_CHECKERBOARD
// #define PROGRAMMABLE_TEST_GRID
// #define PROGRAMMABLE_POINT_GRID

#define MATERIAL_NORMAL mat_makeGlossy(mat_albedo(mat), 0.1, 2.0)
#define MATERIAL_DARK   mat_makeGlossy(mat_albedo(mat) * 0.8, 0.1, 2.0)
#define MATERIAL_BLACK  mat_makeDiffuse(vec3(0.02))

vec2 planeCoords(in Hit hit) {
    vec3 up = abs(hit.normal.z) < (1.0 - EPS) ? vec3(0.0, 0.0, 1.0) : vec3(0.0, 1.0, 0.0);
    vec3 t = normalize(cross(up, hit.normal));
    vec3 b = cross(hit.normal, t);
    return vec2(dot(hit.p, t), dot(hit.p, b));
}

float cubicCatmullRom(float p0, float p1, float p2, float p3, float t) {
    float a = -0.5 * p0 + 1.5 * p1 - 1.5 * p2 + 0.5 * p3;
    float b = p0 - 2.5 * p1 + 2.0 * p2 - 0.5 * p3;
    float c = -0.5 * p0 + 0.5 * p2;
    return ((a * t + b) * t + c) * t + p1;
}

float smoothNoise(vec2 p) {
    vec2 cell = p / SCALE;
    ivec2 pos = ivec2(floor(cell));
    vec2 f = fract(cell);
    ivec2 p0 = pos + ivec2(-1, -1);
    ivec2 p1 = pos + ivec2(-1, 0);
    ivec2 p2 = pos + ivec2(-1, 1);
    ivec2 p3 = pos + ivec2(-1, 2);

    // 4x4 grid of random values for bicubic interpolation
    uint s = initSeed(p0, 0); float r00 = rand(s);
    s = initSeed(p0 + ivec2(1, 0), 0); float r10 = rand(s);
    s = initSeed(p0 + ivec2(2, 0), 0); float r20 = rand(s);
    s = initSeed(p0 + ivec2(3, 0), 0); float r30 = rand(s);

    s = initSeed(p1, 0); float r01 = rand(s);
    s = initSeed(p1 + ivec2(1, 0), 0); float r11 = rand(s);
    s = initSeed(p1 + ivec2(2, 0), 0); float r21 = rand(s);
    s = initSeed(p1 + ivec2(3, 0), 0); float r31 = rand(s);

    s = initSeed(p2, 0); float r02 = rand(s);
    s = initSeed(p2 + ivec2(1, 0), 0); float r12 = rand(s);
    s = initSeed(p2 + ivec2(2, 0), 0); float r22 = rand(s);
    s = initSeed(p2 + ivec2(3, 0), 0); float r32 = rand(s);

    s = initSeed(p3, 0); float r03 = rand(s);
    s = initSeed(p3 + ivec2(1, 0), 0); float r13 = rand(s);
    s = initSeed(p3 + ivec2(2, 0), 0); float r23 = rand(s);
    s = initSeed(p3 + ivec2(3, 0), 0); float r33 = rand(s);

    float cx0 = cubicCatmullRom(r00, r10, r20, r30, f.x);
    float cx1 = cubicCatmullRom(r01, r11, r21, r31, f.x);
    float cx2 = cubicCatmullRom(r02, r12, r22, r32, f.x);
    float cx3 = cubicCatmullRom(r03, r13, r23, r33, f.x);
    return cubicCatmullRom(cx0, cx1, cx2, cx3, f.y);
}

float fade(float t) {
    return 6 * pow(t, 5) - 15 * pow(t, 4) + 10 * pow(t, 3);
}

float perlinNoise(vec2 p) {
    vec2 cell = p / SCALE;
    vec2 f = fract(cell);

    ivec2 p00 = ivec2(floor(cell));
    ivec2 p01 = p00 + ivec2(0, 1);
    ivec2 p10 = p00 + ivec2(1, 0);
    ivec2 p11 = p00 + ivec2(1, 1);

    uint s;
    
    s = initSeed(p00, 0);
    float a00 = rand(s) * 2 * PI;
    vec2 grad00 = vec2(cos(a00), sin(a00));
    s = initSeed(p01, 0);
    float a01 = rand(s) * 2 * PI;
    vec2 grad01 = vec2(cos(a01), sin(a01));
    s = initSeed(p10, 0);
    float a10 = rand(s) * 2 * PI;
    vec2 grad10 = vec2(cos(a10), sin(a10));
    s = initSeed(p11, 0);
    float a11 = rand(s) * 2 * PI;
    vec2 grad11 = vec2(cos(a11), sin(a11));

    float n00 = dot(grad00, f - vec2(0, 0));
    float n01 = dot(grad01, f - vec2(0, 1));
    float n10 = dot(grad10, f - vec2(1, 0));
    float n11 = dot(grad11, f - vec2(1, 1));

    float u = fade(f.x); float v = fade(f.y);
    float nx0 = mix(n00, n10, u);
    float nx1 = mix(n01, n11, u);
    return mix(nx0, nx1, v);
}

float fractalNoise(vec2 p, int octaves, float lacunarity, float gain) {
    uint s = initSeed(ivec2(0), 0);

    const mat2 ROTATION = mat2(0.8, 0.6, -0.6, 0.8);

    float amplitude = 1.0;
    float frequency = 1.0;
    float sum = 0.0;
    float maxAmplitude = 0.0;
    vec2 q = ROTATION * p;
    for (int i = 0; i < octaves; i++) {
        sum += amplitude * perlinNoise(q * frequency + vec2(rand(s), rand(s)) * 152.196);
        maxAmplitude += amplitude;
        frequency *= lacunarity;
        amplitude *= gain;
        q = ROTATION * q;
    }
    return sum / maxAmplitude;
}

Material createProgrammableMaterial(in Material mat, in Hit hit) {
    vec2 p = planeCoords(hit);

#if defined(PROGRAMMABLE_SMOOTH_RANDOM)
    float h = 0.0001;
    int octave = 10;
    float lacunarity = 2.0;
    float gain = 0.5;
    float r = (fractalNoise(p, octave, lacunarity, gain) + 1) / 2;
    // return mat_makeDiffuse(vec3(r));
    float rx = (fractalNoise(p + vec2(h, 0), octave, lacunarity, gain) + 1) / 2;
    float ry = (fractalNoise(p + vec2(0, h), octave, lacunarity, gain) + 1) / 2;
    float dx = (rx - r) / h;
    float dy = (ry - r) / h;
    dx = (dx + 1) / 2;
    dy = (dy + 1) / 2;
    float factor = sqrt(dx*dx + dy*dy);
    return mat_makeDiffuse(mat_albedo(mat) * factor);

    // if (perlinNoise(p) < 0.5) {
    //     return MATERIAL_BLACK;
    // } else {
    //     return MATERIAL_NORMAL;
    // }

#elif defined(PROGRAMMABLE_CHECKERBOARD)
    if (int(round(p.x / SCALE) + round(p.y / SCALE) + 1) % 2 == 0) {
        return MATERIAL_BLACK;
        return MATERIAL_NORMAL;
    } else {
    }

#elif defined(PROGRAMMABLE_TEST_GRID)
    if (abs(p.x - round(p.x / SCALE) * SCALE) < FEATHER || abs(p.y - round(p.y / SCALE) * SCALE) < FEATHER) {
        return MATERIAL_BLACK;
    } else if (int(round(p.x / SCALE + 0.5) + round(p.y / SCALE + 0.5) + 1) % 2 == 0) {
        if (abs(p.x - round(p.x / SCALE * 2) * SCALE / 2) < SMALL_FEATHER || abs(p.y - round(p.y / SCALE * 2) * SCALE / 2) < SMALL_FEATHER) {
            return MATERIAL_BLACK;
        }
        return MATERIAL_NORMAL;
    }
    return MATERIAL_DARK;

#elif defined(PROGRAMMABLE_POINT_GRID)
    float x = abs(p.x - round(p.x / SCALE) * SCALE);
    float y = abs(p.y - round(p.y / SCALE) * SCALE);
    if (x*x + y*y < 0.08*0.08) {
        return MATERIAL_NORMAL;
    }
    return MATERIAL_BLACK;

#endif
}

#endif // PROGRAMMABLE_GLSL
