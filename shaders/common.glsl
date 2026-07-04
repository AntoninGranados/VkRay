#ifndef COMMON_GLSL
#define COMMON_GLSL

#define mat_Principled   0
#define mat_Emissive     1
#define mat_Lambertian   2
#define mat_GgxMetal     3
#define mat_GgxGlossy    4
#define mat_Dielectric   5
#define mat_Volume       6
#define mat_Programmable 7

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
    float varianceProba;
    uint  selectionMask;
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
#define debug_SelectionMask 11
#define debug_SkyMask       12

#endif
