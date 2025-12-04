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

layout(set = 0, binding = 1) uniform sampler2D prevTex;

layout(set = 0, binding = 2) buffer readonly SphereBuffer {
    Sphere spheres[];
} sphereBuffer;
layout(set = 0, binding = 3) buffer readonly PlaneBuffer {
    Plane planes[];
} planeBuffer;
layout(set = 0, binding = 4) buffer readonly BoxBuffer {
    Box boxes[];
} boxBuffer;
layout(set = 0, binding = 5) buffer readonly ObjectBuffer {
    uint objectCount;
    int selectedObjectId;
    Object objects[];
} objectBuffer;

#endif
