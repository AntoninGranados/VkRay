#ifndef COMMON_GLSL
#define COMMON_GLSL

#include "core/materials/generated/material_types.glsl"

float luma(vec3 c) {
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

struct AOVBuffer {
    uint  hitValid;
    vec3  positionW;
    vec3  position;
    vec3  normalW;
    vec2  normal;
    vec3  albedo;
    float roughness;
    uint  matType;
    uint  skyMask;
};

struct PixelInfo {
    AOVBuffer aov;
    float mean;
    float m2;
    int   count;
    uint  bounces;
    uint  bvhChecks;
    uint  triangleChecks;
    float varianceProba;
};

#define debug_None          0
#define debug_PositionW     1
#define debug_Position      2
#define debug_NormalW       3
#define debug_Normal        4
#define debug_Albedo        5
#define debug_Roughness     6
#define debug_MatType       7
#define debug_Bounces       8
#define debug_HitChecks     9
#define debug_Variance      10
#define debug_SkyMask       11

#endif
