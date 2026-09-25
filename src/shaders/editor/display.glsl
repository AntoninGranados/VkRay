#version 450

#include "../common.glsl"

#define INPUTS_GLSL
#include "../core/utils.glsl"
#include "../core/materials/material_utils.glsl"

layout(set = 0, binding = 0) uniform sampler2D outputTex;
layout(set = 0, binding = 1) buffer PixelInfoBuffer {
    PixelInfo pixels[];
} pixelInfoBuffer;
layout(set = 0, binding = 2, rgba32f) uniform writeonly image2D displayOut;
layout(set = 0, binding = 3) uniform DisplayUBO {
    int  showFocusPlane;
    int  selectedObjectId;
    int  previewBorderEnabled;
    vec2 previewFrameExtent;
    vec4 focusPlane;
    CameraUBO camera;
} ubo;
layout(set = 0, binding = 4) buffer readonly VertexBuffer { Vertex   vertices[]; } vertexBuffer;
layout(set = 0, binding = 5) buffer readonly IndexBuffer  { uint     indices[];  } indexBuffer;
layout(set = 0, binding = 6) buffer readonly BvhBuffer    { BvhNode  bvhNodes[]; } bvhBuffer;
layout(set = 0, binding = 7) buffer readonly MeshBuffer   { Mesh     meshes[];   } meshBuffer;
layout(set = 0, binding = 8) buffer readonly ObjectBuffer { uint objectCount; Object objects[]; } objectBuffer;
layout(set = 0, binding = 9) buffer readonly MaterialBuffer { Material materials[]; } materialBuffer;
layout(set = 0, binding = 10) buffer readonly MotionBuffer  { MotionSample samples[]; } motionBuffer;
layout(set = 0, binding = 11) buffer readonly PluginParamsBuffer { float values[]; } pluginParams;

#include "../core/global.glsl"
#include "../core/camera/camera.glsl"

layout(local_size_x = 8, local_size_y = 8) in;

const float outlineWidth = 3.0;
const float previewStripeSpacing = 64.0;
const float feather      = 0.4;
const vec3  edgeColor    = vec3(1.0, 0.5, 0.062);
const vec3  focusColor   = vec3(1.0, 0.3, 1.0);

PixelInfo samplePixelInfo(ivec2 vpCoord, ivec2 vpSize, ivec2 renderSize) {
    ivec2 rc = clamp(vpCoord * renderSize / vpSize, ivec2(0), renderSize - ivec2(1));
    return pixelInfoBuffer.pixels[uint(rc.y * renderSize.x + rc.x)];
}

Ray viewportRay(ivec2 coord, ivec2 viewportSize) {
    vec2 ndc = (vec2(coord) + 0.5) / vec2(viewportSize) * 2.0 - 1.0;
    RngState rng = initRngState(uvec2(coord), 0u);
    return getRay(ndc, rng);
}

void main() {
    ivec2 coord        = ivec2(gl_GlobalInvocationID.xy);
    ivec2 viewportSize = imageSize(displayOut);
    if (coord.x >= viewportSize.x || coord.y >= viewportSize.y) return;

    ivec2 renderSize  = textureSize(outputTex, 0);
    ivec2 renderCoord = clamp(coord * renderSize / viewportSize, ivec2(0), renderSize - ivec2(1));

    vec3 color = texelFetch(outputTex, renderCoord, 0).rgb;

    bool isSelected = false;
    if (ubo.selectedObjectId >= 0) {
        Ray selRay = viewportRay(coord, viewportSize);
        Statistics dummy = Statistics(0u, 0u);
        Hit selHit = rayObjectIntersection(selRay, objectBuffer.objects[uint(ubo.selectedObjectId)], false, INFINITY, dummy);
        isSelected = foundIntersection(selHit);
    }

    uint centerMask = isSelected ? 1u : 0u;
    int stepPx = int(outlineWidth);
    float neighborMask = 0.0;
    int count = 0;
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            if (i == 0 && j == 0) continue;
            ivec2 nb = coord + ivec2(i, j) * stepPx;
            if (nb.x < 0 || nb.x >= viewportSize.x || nb.y < 0 || nb.y >= viewportSize.y) continue;
            if (ubo.selectedObjectId >= 0) {
                Ray nbRay = viewportRay(nb, viewportSize);
                Statistics dummy2 = Statistics(0u, 0u);
                Hit nbHit = rayObjectIntersection(nbRay, objectBuffer.objects[uint(ubo.selectedObjectId)], true, INFINITY, dummy2);
                neighborMask += foundIntersection(nbHit) ? 1.0 : 0.0;
            }
            count++;
        }
    }
    if (count > 0) neighborMask /= float(count);

    float edgeAmount = centerMask != 0u ? 1.0 - neighborMask : neighborMask;
    color = mix(color, edgeColor, smoothstep(0.0, feather, edgeAmount));

    if (ubo.showFocusPlane != 0) {
        float t;
        PixelInfo pix = samplePixelInfo(coord, viewportSize, renderSize);
        if (pix.aov.hitValid != 0u) {
            float signedDist = dot(pix.aov.positionW, ubo.focusPlane.xyz) + ubo.focusPlane.w;
            t = signedDist > 0.0 ? 1.0 : 0.0;
        } else {
            Ray skyRay = viewportRay(coord, viewportSize);
            float denom = dot(skyRay.dir, ubo.focusPlane.xyz);
            float t_hit = denom != 0.0
                ? -(dot(skyRay.origin, ubo.focusPlane.xyz) + ubo.focusPlane.w) / denom
                : -1.0;
            t = t_hit > 0.0 ? 1.0 : 0.0;
        }
        color *= mix(vec3(1.0), focusColor, t);
    }

    if (ubo.previewBorderEnabled != 0) {
        vec2 pixelCoord = vec2(coord) + 0.5;
        vec2 frameHalfPx = vec2(viewportSize) * 0.5 * ubo.previewFrameExtent;
        vec2 centeredPx = abs(pixelCoord - vec2(viewportSize) * 0.5);
        vec2 outsideDist2 = centeredPx - frameHalfPx;
        float outsideDist = max(outsideDist2.x, outsideDist2.y);

        if (outsideDist > 0.0) {
            color *= 0.2;
            if (fract((pixelCoord.x + pixelCoord.y) / previewStripeSpacing) < 0.5) color *= 0.6;
        }

        float edgeMask = 1.0 - smoothstep(outlineWidth - feather, outlineWidth + feather, abs(outsideDist));
        color = mix(color, edgeColor, edgeMask);
    }

    imageStore(displayOut, coord, vec4(color, 1.0));
}
