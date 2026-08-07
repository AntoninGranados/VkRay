#ifndef INPUTS_GLSL
#define INPUTS_GLSL

#include "utils.glsl"
#include "materials/materials.glsl"

struct CameraUBO {
    vec3 eye;
    vec3 U;
    vec3 V;
    vec3 W;
    float lensRadius;
    float focusDistance;
};

struct ScreenUBO {
    vec2 size;
    float aspect;
};

struct RenderUBO {
    Enum lightMode;
    int maxBounces;
    int importanceSampling;
    int clipAccumulation;
    float clipThreshold;
    int varianceSampling;
    int varianceWarmupSamples;
};

layout(std140, set = 0, binding = 0) uniform UBO {
    int       sampleCount;
    int       selectedObjectId;
    CameraUBO camera;
    ScreenUBO screen;
    RenderUBO render;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D prevTex;

layout(set = 0, binding = 2) buffer PixelInfoBuffer {
    PixelInfo pixels[];
} pixelInfoBuffer;
layout(set = 0, binding = 3) buffer readonly SphereBuffer {
    Sphere spheres[];
} sphereBuffer;
layout(set = 0, binding = 4) buffer readonly PlaneBuffer {
    Plane planes[];
} planeBuffer;
layout(set = 0, binding = 5) buffer readonly BoxBuffer {
    Box boxes[];
} boxBuffer;
layout(set = 0, binding = 6) buffer readonly VertexBuffer {
    Vertex vertices[];
} vertexBuffer;
layout(set = 0, binding = 7) buffer readonly IndexBuffer {
    uint indices[];
} indexBuffer;
layout(set = 0, binding = 8) buffer readonly BvhBuffer {
    BvhNode bvhNodes[];
} bvhBuffer;
layout(set = 0, binding = 9) buffer readonly MeshBuffer {
    Mesh meshes[];
} meshBuffer;
layout(set = 0, binding = 10) buffer readonly MaterialBuffer {
    Material materials[];
} materialBuffer;
layout(set = 0, binding = 11) buffer readonly ObjectBuffer {
    uint objectCount;
    Object objects[];
} objectBuffer;
layout(set = 0, binding = 12) buffer readonly LightBuffer {
    float totalArea;
    Light lights[];
} lightBuffer;
layout(set = 0, binding = 13) buffer readonly QuadBuffer {
    Quad quads[];
} quadBuffer;

layout(set = 0, binding = 15) uniform sampler2D lensSampler;

#endif
