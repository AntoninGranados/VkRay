#ifndef INPUTS_GLSL
#define INPUTS_GLSL

#include "objects.glsl"
#include "materials.glsl"

layout(location = 0) in vec2 fragPos;
layout(location = 0) out vec4 outColor;

layout(std140, set = 0, binding = 0) uniform UBO {
    vec3 cameraPos;
    vec3 cameraDir;

    vec2 screenSize;
    float aspect;
    float tanHFov;
    
    int frameCount;
    float time;

    int timeOfDay;
} ubo;
layout(set = 0, binding = 1) buffer readonly SSBO {
    uint sphereCount;
    int selectedSphereId;
    Sphere spheres[];
} ssbo;
/*
layout(set = 0, binding = 1) buffer readonly SSBO {
    uint objectCount;
    int selectedObjectId;
    Object objects[];
    Sphere spheres[];
    Plane planes[];
    Box boxes[];
} ssbo;
*/
layout(set = 0, binding = 2) uniform sampler2D prevTex;

#define PLANE_COUNT 10
uint planeCount = 0;
Plane planes[PLANE_COUNT];

#define BOX_COUNT 10
uint boxCount = 0;
Box boxes[BOX_COUNT];

#endif
