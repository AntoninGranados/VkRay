#version 450

#include "pixel_info.glsl"

layout(set = 0, binding = 0) uniform sampler2D tex;
layout(set = 0, binding = 1) uniform UBO {
    int frameCount;
    float resolution;
    int debugView;
    int previewBorderEnabled;
    int denoisingEnabled;
} ubo;
layout(set = 0, binding = 2) buffer PixelInfoBuffer {
    PixelInfo pixels[];
} pixelInfoBuffer;

layout(location = 0) in vec2 fragPos;
layout(location = 0) out vec4 outColor;

const float outlineWidth = 2.0;
const float feather = 0.4;
#define debug_None          0
#define debug_Bounces       1
#define debug_Normal        2
#define debug_Position      3
#define debug_Diffuse       4
#define debug_SelectionMask 5
#define debug_Variance      6

float luma(vec3 c) {
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

ivec2 blockCoordFromResolution(ivec2 pixelCoord, vec2 screenCoord, ivec2 texSize, float resolution) {
    ivec2 blockCoord = pixelCoord;
    if (resolution > 1.0) {
        blockCoord = ivec2(floor(screenCoord / resolution) * resolution);
    }
    return clamp(blockCoord, ivec2(0), texSize - ivec2(1));
}

vec3 visualizeVariance(vec2 uv, vec2 texSize) {
    vec2 screenCoord = uv * texSize;
    ivec2 pixelCoord = ivec2(screenCoord);
    ivec2 blockCoord = blockCoordFromResolution(pixelCoord, screenCoord, ivec2(texSize), ubo.resolution);
    uint index = uint(blockCoord.y * int(texSize.x) + blockCoord.x);
    PixelInfo pixelInfo = pixelInfoBuffer.pixels[index];
    
    return vec3(pixelInfo.varianceProba);
}

vec3 visualizeSelectionMask(uint mask) {
    return vec3(mask != 0u ? 1.0 : 0.0);
}

vec3 visualizeNormal(PixelInfo pixelInfo) {
    if (pixelInfo.aov.hitValid == 0u) return vec3(0.0);
    return normalize(pixelInfo.aov.normal) * 0.5 + 0.5;
}

vec3 visualizePosition(PixelInfo pixelInfo) {
    if (pixelInfo.aov.hitValid == 0u) return vec3(0.0);
    return clamp(pixelInfo.aov.position, 0.0, 1.0);
}

vec3 visualizeDiffuse(PixelInfo pixelInfo) {
    if (pixelInfo.aov.hitValid == 0u) return vec3(0.0);
    return clamp(pixelInfo.aov.albedo, 0.0, 1.0);
}

const vec3 edgeColor = vec3(1.0, 0.5, 0.062);

void main() {
    vec2 uv = fragPos * 0.5 + 0.5;

    vec2 texSize = vec2(textureSize(tex, 0));
    vec2 texelSize = 1.0 / texSize;

    vec2 screenCoord = uv * texSize;
    ivec2 pixelCoord = ivec2(screenCoord);
    ivec2 blockCoord = blockCoordFromResolution(pixelCoord, screenCoord, ivec2(texSize), ubo.resolution);
    vec3 color = texelFetch(tex, pixelCoord, 0).rgb;

    float targetMin = 0.5;
    float targetMax = 1.5;

    uint blockIndex = uint(blockCoord.y * int(texSize.x) + blockCoord.x);
    PixelInfo blockPixelInfo = pixelInfoBuffer.pixels[blockIndex];

    uint centerIndex = uint(pixelCoord.y * int(texSize.x) + pixelCoord.x);
    PixelInfo centerPixelInfo = pixelInfoBuffer.pixels[centerIndex];
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

    if (ubo.debugView == debug_Variance) {
        outColor = vec4(visualizeVariance(uv, texSize), 1.0);
        return;
    }

    if (ubo.debugView == debug_Normal) {
        outColor = vec4(visualizeNormal(blockPixelInfo), 1.0);
        return;
    }

    if (ubo.debugView == debug_Position) {
        outColor = vec4(visualizePosition(blockPixelInfo), 1.0);
        return;
    }

    if (ubo.debugView == debug_Diffuse) {
        outColor = vec4(visualizeDiffuse(blockPixelInfo), 1.0);
        return;
    }

    if (ubo.debugView == debug_SelectionMask) {
        outColor = vec4(visualizeSelectionMask(centerMask), 1.0);
        return;
    }

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
