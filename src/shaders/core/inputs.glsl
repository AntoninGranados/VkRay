#ifndef INPUTS_GLSL
#define INPUTS_GLSL

#include "utils.glsl"
#include "materials/material_utils.glsl"

struct ScreenUBO {
    vec2 size;
    float aspect;
};

struct RenderUBO {
    int skyParamsBase;
    int maxBounces;
    int importanceSampling;
    int clipAccumulation;
    float clipThreshold;
    int varianceSampling;
    int varianceWarmupSamples;
};

layout(std140, set = 0, binding = 0) uniform UBO {
    int       sampleCount;
    CameraUBO camera;
    ScreenUBO screen;
    RenderUBO render;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D prevTex;

layout(set = 0, binding = 2) buffer PixelInfoBuffer {
    PixelInfo pixels[];
} pixelInfoBuffer;
layout(set = 0, binding = 3) buffer readonly VertexBuffer {
    Vertex vertices[];
} vertexBuffer;
layout(set = 0, binding = 4) buffer readonly IndexBuffer {
    uint indices[];
} indexBuffer;
layout(set = 0, binding = 5) buffer readonly BvhBuffer {
    BvhNode bvhNodes[];
} bvhBuffer;
layout(set = 0, binding = 6) buffer readonly MeshBuffer {
    Mesh meshes[];
} meshBuffer;
layout(set = 0, binding = 7) buffer readonly MaterialBuffer {
    Material materials[];
} materialBuffer;
layout(set = 0, binding = 8) buffer readonly PluginParamsBuffer {
    float values[];
} pluginParams;
layout(set = 0, binding = 9) buffer readonly ObjectBuffer {
    uint objectCount;
    Object objects[];
} objectBuffer;
layout(set = 0, binding = 10) buffer readonly LightBuffer {
    float totalArea;
    Light lights[];
} lightBuffer;

layout(rgba32f, set = 0, binding = 11) writeonly uniform image2D outputImage;

layout(set = 0, binding = 12) uniform sampler2D lensSampler;

layout(set = 0, binding = 13) buffer readonly MotionBuffer {
    MotionSample samples[];
} motionBuffer;

ResolvedMaterial unpackMaterial(in Material mat) {
    ResolvedMaterial resolved;
    resolved.type = mat.type;
    for (int i = 0; i < MATERIAL_PAYLOAD_SIZE; i++) resolved.payload[i] = pluginParams.values[mat.base + i];
    return resolved;
}

#endif
