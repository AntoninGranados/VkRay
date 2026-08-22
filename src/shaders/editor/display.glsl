#version 450

#include "../common.glsl"

#define INPUTS_GLSL
#include "../core/utils.glsl"
#include "../core/materials/materials.glsl"

layout(set = 0, binding = 0) uniform sampler2D outputTex;
layout(set = 0, binding = 1) buffer PixelInfoBuffer {
    PixelInfo pixels[];
} pixelInfoBuffer;
layout(set = 0, binding = 2, rgba32f) uniform writeonly image2D displayOut;
layout(set = 0, binding = 3) uniform DisplayUBO {
    int  showFocusPlane;
    int  selectedObjectId;
    int  previewBorderEnabled;
    vec4 focusPlane;
    vec3 cameraEye;
    vec3 cameraU;
    vec3 cameraV;
    vec3 cameraW;
} displayUBO;
layout(set = 0, binding = 4)  buffer readonly SphereBuffer { Sphere   spheres[];  } sphereBuffer;
layout(set = 0, binding = 5)  buffer readonly PlaneBuffer  { Plane    planes[];   } planeBuffer;
layout(set = 0, binding = 6)  buffer readonly BoxBuffer    { Box      boxes[];    } boxBuffer;
layout(set = 0, binding = 7)  buffer readonly VertexBuffer { Vertex   vertices[]; } vertexBuffer;
layout(set = 0, binding = 8)  buffer readonly IndexBuffer  { uint     indices[];  } indexBuffer;
layout(set = 0, binding = 9)  buffer readonly BvhBuffer    { BvhNode  bvhNodes[]; } bvhBuffer;
layout(set = 0, binding = 10) buffer readonly MeshBuffer   { Mesh     meshes[];   } meshBuffer;
layout(set = 0, binding = 11) buffer readonly ObjectBuffer { uint objectCount; Object objects[]; } objectBuffer;
layout(set = 0, binding = 12) buffer readonly QuadBuffer    { Quad     quads[];    } quadBuffer;
layout(set = 0, binding = 13) buffer readonly MaterialBuffer { Material materials[]; } materialBuffer;

#include "../core/global.glsl"

layout(local_size_x = 8, local_size_y = 8) in;

const float outlineWidth = 2.0;
const float feather      = 0.4;
const vec3  edgeColor    = vec3(1.0, 0.5, 0.062);
const vec3  focusColor   = vec3(1.0, 0.3, 1.0);

PixelInfo samplePixelInfo(ivec2 vpCoord, ivec2 vpSize, ivec2 renderSize) {
    ivec2 rc = clamp(vpCoord * renderSize / vpSize, ivec2(0), renderSize - ivec2(1));
    return pixelInfoBuffer.pixels[uint(rc.y * renderSize.x + rc.x)];
}

Ray viewportRay(ivec2 coord, ivec2 viewportSize) {
    vec2 ndc = (vec2(coord) + 0.5) / vec2(viewportSize) * 2.0 - 1.0;
    vec3 dir = normalize(ndc.x * displayUBO.cameraU - ndc.y * displayUBO.cameraV + displayUBO.cameraW);
    return Ray(displayUBO.cameraEye, dir);
}

void main() {
    ivec2 coord        = ivec2(gl_GlobalInvocationID.xy);
    ivec2 viewportSize = imageSize(displayOut);
    if (coord.x >= viewportSize.x || coord.y >= viewportSize.y) return;

    ivec2 renderSize  = textureSize(outputTex, 0);
    ivec2 renderCoord = clamp(coord * renderSize / viewportSize, ivec2(0), renderSize - ivec2(1));

    vec3 color = texelFetch(outputTex, renderCoord, 0).rgb;

    bool isSelected = false;
    if (displayUBO.selectedObjectId >= 0) {
        Ray selRay = viewportRay(coord, viewportSize);
        Statistics dummy = Statistics(0u, 0u);
        Hit selHit = rayObjectIntersection(selRay, objectBuffer.objects[uint(displayUBO.selectedObjectId)], false, INFINITY, dummy);
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
            if (displayUBO.selectedObjectId >= 0) {
                Ray nbRay = viewportRay(nb, viewportSize);
                Statistics dummy2 = Statistics(0u, 0u);
                Hit nbHit = rayObjectIntersection(nbRay, objectBuffer.objects[uint(displayUBO.selectedObjectId)], true, INFINITY, dummy2);
                neighborMask += foundIntersection(nbHit) ? 1.0 : 0.0;
            }
            count++;
        }
    }
    if (count > 0) neighborMask /= float(count);

    float edgeAmount = centerMask != 0u ? 1.0 - neighborMask : neighborMask;
    color = mix(color, edgeColor, smoothstep(0.0, feather, edgeAmount));

    if (displayUBO.showFocusPlane != 0) {
        float t;
        PixelInfo pix = samplePixelInfo(coord, viewportSize, renderSize);
        if (pix.aov.hitValid != 0u) {
            float signedDist = dot(pix.aov.positionW, displayUBO.focusPlane.xyz) + displayUBO.focusPlane.w;
            t = signedDist > 0.0 ? 1.0 : 0.0;
        } else {
            Ray skyRay = viewportRay(coord, viewportSize);
            float denom = dot(skyRay.dir, displayUBO.focusPlane.xyz);
            float t_hit = denom != 0.0
                ? -(dot(skyRay.origin, displayUBO.focusPlane.xyz) + displayUBO.focusPlane.w) / denom
                : -1.0;
            t = t_hit > 0.0 ? 1.0 : 0.0;
        }
        color *= mix(vec3(1.0), focusColor, t);
    }

    if (displayUBO.previewBorderEnabled != 0) {
        vec2 ndc = (vec2(coord) + 0.5) / vec2(viewportSize) * 2.0 - 1.0;
        float dist = max(abs(ndc.x), abs(ndc.y));
        color *= dist > 0.8 ? 0.2 : 1.0;
        float borderHalfWidth = outlineWidth / float(min(viewportSize.x, viewportSize.y));
        float edgeMask = 1.0 - smoothstep(0.0, borderHalfWidth, abs(dist - 0.8));
        color = mix(color, edgeColor, edgeMask);
    }

    imageStore(displayOut, coord, vec4(color, 1.0));
}
