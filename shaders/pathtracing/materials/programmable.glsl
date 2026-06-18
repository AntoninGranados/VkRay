#ifndef PROGRAMMABLE_GLSL
#define PROGRAMMABLE_GLSL

#include "../utils.glsl"
#include "../random.glsl"

#include "material_utils.glsl"
#include "lambertian.glsl"
#include "ggx_metal.glsl"
#include "ggx_glossy.glsl"

#define SCALE 0.2

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
        return Material(mat_Lambertian, mat.albedo * 0.6, 0.0, 0.0, 0.0);
    } else {
        // return Material(mat_Lambertian, mat.albedo, 0.0, 0.0, 0.0);
        return Material(mat_GgxGlossy, mat.albedo, 0.1, 0.6, 0.0);
    }

    // if (int(round(p.x / SCALE) + round(p.y / SCALE) + 1) % 2 == 0) {
    //     return Material(mat_Lambertian, mat.albedo * 0.8, 0.0, 0.0, 0.0);
    // } else {
    //     return Material(mat_GgxGlossy, mat.albedo, 0.1, 0.2, 0.0);
    // }

}

#endif // PROGRAMMABLE_GLSL
