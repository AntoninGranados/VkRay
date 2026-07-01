#ifndef PIXEL_INFO_GLSL
#define PIXEL_INFO_GLSL

struct AOVBuffer {
    vec3  normal;
    vec3  position;
    vec3  albedo;
    vec3  albedoOpaque;
    vec2  camNormal;
    vec2  camNormalOpaque;
    float depth;
    float depthOpaque;
    uint  skyMask;
    uint  skyMaskOpaque;
    uint  hitValid;
    uint  opaqueHitValid;
};

struct PixelInfo {
    AOVBuffer aov;
    float mean;
    float m2;
    int   count;
    float varianceProba;
    uint  selectionMask;
};

#endif
