#version 450

#include "common.glsl"

layout(set = 0, binding = 0) uniform sampler2D tex;
layout(set = 0, binding = 1) uniform UBO {
    int debugView;
    int previewBorderEnabled;
} ubo;
layout(set = 0, binding = 2) buffer PixelInfoBuffer {
    PixelInfo pixels[];
} pixelInfoBuffer;

layout(location = 0) in vec2 fragPos;
layout(location = 0) out vec4 outColor;

const float outlineWidth = 2.0;
const float feather = 0.4;

float luma(vec3 c) {
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

vec3 visualizeVariance(vec2 uv, vec2 texSize) {
    ivec2 pixelCoord = ivec2(uv * texSize);
    uint index = uint(pixelCoord.y * int(texSize.x) + pixelCoord.x);
    PixelInfo pixelInfo = pixelInfoBuffer.pixels[index];
    
    return vec3(pixelInfo.varianceProba);
}

vec3 visualizeSelectionMask(uint mask) {
    return vec3(mask != 0u ? 1.0 : 0.0);
}

vec3 visualize(vec3 v)  { return normalize(v) * 0.5 + 0.5; }
vec3 visualize(vec2 v)  { float z = sqrt(max(0.0, 1.0 - dot(v, v))); return vec3(v, z) * 0.5 + 0.5; }
vec3 visualize(float v) { return vec3(v); }
vec3 visualize(uint v)  { return vec3(v != 0u ? 1.0 : 0.0); }

vec3 visualizeMatType(uint t) {
    if (t == mat_Principled)   return vec3(0.8, 0.8, 0.8);
    if (t == mat_Emissive)     return vec3(1.0, 1.0, 0.2);
    if (t == mat_Lambertian)   return vec3(0.9, 0.5, 0.2);
    if (t == mat_GgxMetal)     return vec3(0.9, 0.8, 0.1);
    if (t == mat_GgxGlossy)    return vec3(0.2, 0.5, 0.9);
    if (t == mat_Dielectric)   return vec3(0.2, 0.9, 0.8);
    if (t == mat_Volume)       return vec3(0.7, 0.2, 0.9);
    if (t == mat_Programmable) return vec3(0.9, 0.2, 0.7);
    return vec3(0.0);
}

const vec3 edgeColor = vec3(1.0, 0.5, 0.062);

void main() {
    vec2 uv = fragPos * 0.5 + 0.5;

    vec2 texSize = vec2(textureSize(tex, 0));

    ivec2 pixelCoord = ivec2(uv * texSize);
    vec3 color = texelFetch(tex, pixelCoord, 0).rgb;

    float targetMin = 0.5;
    float targetMax = 1.5;

    uint pixelIndex = uint(pixelCoord.y * int(texSize.x) + pixelCoord.x);
    PixelInfo blockPixelInfo = pixelInfoBuffer.pixels[pixelIndex];
    PixelInfo centerPixelInfo = blockPixelInfo;
    uint centerMask = centerPixelInfo.selectionMask;
    int stepPx = int(outlineWidth);

    float neighborMask = 0.0;
    int count = 0;
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            if (i == 0 && j == 0) continue;

            ivec2 sampleCoord = pixelCoord + ivec2(i, j) * stepPx;
            if (sampleCoord.x < 0 || sampleCoord.x >= int(texSize.x)) continue;
            if (sampleCoord.y < 0 || sampleCoord.y >= int(texSize.y)) continue;

            uint sampleIndex = uint(sampleCoord.y * int(texSize.x) + sampleCoord.x);
            neighborMask += pixelInfoBuffer.pixels[sampleIndex].selectionMask != 0u ? 1.0 : 0.0;
            count += 1;
        }
    }
    neighborMask /= count;

    float edgeAmount = centerMask != 0u ? 1.0 - neighborMask : neighborMask;
    float outline = smoothstep(0.0, feather, edgeAmount);
    color = mix(color, edgeColor, outline);

#define HIT(expr) (blockPixelInfo.aov.hitValid != 0u ? (expr) : vec3(0.0))

    if (ubo.debugView == debug_PositionW)     { outColor = vec4(HIT(visualize(blockPixelInfo.aov.positionW)), 1.0); return; }
    if (ubo.debugView == debug_Position)      { vec3 p = blockPixelInfo.aov.position; vec3 vis; vis.xy = p.xy / (1.0 + abs(p.xy)) * 0.5 + 0.5; vis.z = p.z / (1.0 + p.z); outColor = vec4(HIT(vis), 1.0); return; }
    if (ubo.debugView == debug_NormalW)       { outColor = vec4(HIT(visualize(blockPixelInfo.aov.normalW)),   1.0); return; }
    if (ubo.debugView == debug_Normal)        { outColor = vec4(HIT(visualize(blockPixelInfo.aov.normal)),    1.0); return; }
    if (ubo.debugView == debug_Albedo)        { outColor = vec4(HIT(clamp(blockPixelInfo.aov.albedo, 0.0, 1.0)), 1.0); return; }
    if (ubo.debugView == debug_Roughness)     { outColor = vec4(HIT(visualize(blockPixelInfo.aov.roughness)), 1.0); return; }
    if (ubo.debugView == debug_MatType)       { outColor = vec4(HIT(visualizeMatType(blockPixelInfo.aov.matType)), 1.0); return; }
    if (ubo.debugView == debug_Variance)      { outColor = vec4(visualizeVariance(uv, texSize), 1.0); return; }
    if (ubo.debugView == debug_SelectionMask) { outColor = vec4(visualizeSelectionMask(centerMask), 1.0); return; }
    if (ubo.debugView == debug_SkyMask)       { outColor = vec4(visualize(blockPixelInfo.aov.skyMask), 1.0); return; }

    if (ubo.previewBorderEnabled != 0) {
        if (abs(fragPos.x) > 0.8 || abs(fragPos.y) > 0.8) {
            color *= 0.2;
        }
        if ((abs(fragPos.x) < 0.8 + 2*outlineWidth/texSize.x && abs(fragPos.x) > 0.8 - 2*outlineWidth/texSize.x) && abs(fragPos.y) < 0.8) {
            color = edgeColor;
        }
        if ((abs(fragPos.y) < 0.8 + 2*outlineWidth/texSize.y && abs(fragPos.y) > 0.8 - 2*outlineWidth/texSize.y) && abs(fragPos.x) < 0.8) {
            color = edgeColor;
        }
    }

    outColor = vec4(color, 1.0);
}
