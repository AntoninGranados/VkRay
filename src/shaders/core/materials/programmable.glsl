#ifndef PROGRAMMABLE_GLSL
#define PROGRAMMABLE_GLSL

#include "../utils.glsl"
#include "../random.glsl"

#include "material_utils.glsl"
#include "lambertian.glsl"
#include "ggx_metal.glsl"
#include "ggx_glossy.glsl"

#define SCALE 1.0
#define FEATHER 0.02
#define SMALL_FEATHER 0.01

// #define PROGRAMMABLE_SMOOTH_RANDOM
// #define PROGRAMMABLE_CHECKERBOARD
#define PROGRAMMABLE_TEST_GRID
// #define PROGRAMMABLE_POINT_GRID

#define MATERIAL_NORMAL Material(mat_GgxGlossy, mat.albedo, 0.1, 0.0, 2.0, 0.0, 0.0, 1.0, 0.0, 1.0);
#define MATERIAL_DARK   Material(mat_GgxGlossy, mat.albedo*0.8, 0.1, 0.0, 2.0, 0.0, 0.0, 1.0, 0.0, 1.0);
#define MATERIAL_BLACK  Material(mat_Lambertian, vec3(0.02), 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 1.0);

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

Material createProgrammableMaterial(in Material mat, in Hit hit) {
    vec2 p = planeCoords(hit);

#if defined(PROGRAMMABLE_SMOOTH_RANDOM)
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
    float r = clamp(cubicCatmullRom(cx0, cx1, cx2, cx3, f.y), 0.0, 1.0);

    if (r < 0.5) {
        return MATERIAL_BLACK;
        return MATERIAL_NORMAL;
    } else {
    }

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

    // if (abs(p.x - round(p.x / SCALE) * SCALE) < FEATHER || abs(p.y - round(p.y / SCALE) * SCALE) < FEATHER) {
    //     return MATERIAL_BLACK;
    // }
    // } else if (int(round(p.x / SCALE + 0.5) + round(p.y / SCALE + 0.5) + 1) % 2 == 0) {
    //     if (abs(p.x - round(p.x / SCALE * 2) * SCALE / 2) < SMALL_FEATHER || abs(p.y - round(p.y / SCALE * 2) * SCALE / 2) < SMALL_FEATHER) {
    //         return MATERIAL_BLACK;
    //     return MATERIAL_NORMAL;
    //     }
    // return MATERIAL_DARK;
    // }

#endif
}

#endif // PROGRAMMABLE_GLSL
