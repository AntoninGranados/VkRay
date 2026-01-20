#version 450

struct PixelInfo {
    float mean;
    float m2;
    float count;
    float varianceProba;
};

layout(set = 0, binding = 0) uniform sampler2D tex;
layout(set = 0, binding = 1) uniform UBO {
    int frameCount;
    float resolution;
    int debugView;
} ubo;
layout(set = 0, binding = 2) buffer PixelInfoBuffer {
    PixelInfo pixels[];
} pixelInfoBuffer;

layout(location = 0) in vec2 fragPos;
layout(location = 0) out vec4 outColor;

const float outlineWidth = 3.0;
const float feather = 0.8;
#define debug_None          0
#define debug_Bounces       1
#define debug_Normal        2
#define debug_SelectionMask 3
#define debug_Variance      4

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

    // return vec3(pixelInfo.count / ubo.frameCount);
}

vec3 visualizeSelectionMask(float alpha) {
    return vec3(step(0.5, alpha));
}

void main() {
    vec2 uv = fragPos * 0.5 + 0.5;

    vec4 texelData = texture(tex, uv);
    vec2 texSize = vec2(textureSize(tex, 0));
    vec2 texelSize = 1.0 / texSize;

    vec3 color = texelData.rgb;
    if (ubo.frameCount <= 1) {
        vec2 screenCoord = uv * texSize;
        ivec2 blockCoord = ivec2(floor(screenCoord / ubo.resolution) * ubo.resolution);
        blockCoord = clamp(blockCoord, ivec2(0), ivec2(texSize) - ivec2(1));
        color = texelFetch(tex, blockCoord, 0).rgb;
    }

    float targetMin = 0.5;
    float targetMax = 1.5;

    float centerAlpha = texelData.a;
    float centerMask = step(targetMin, centerAlpha) - step(targetMax, centerAlpha);
    vec2 stepV = texelSize * outlineWidth;

    float neighborMask = 0.0;
    int count = 0;
    vec2 samplePos;
    float sampleAlpha;
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            if (i == 0 && j == 0) continue;
            
            samplePos = uv + vec2(i, j) * stepV;
            sampleAlpha = texture(tex, samplePos).a;
            if (0 > samplePos.x || samplePos.x > 1) sampleAlpha = 0.0;
            if (0 > samplePos.y || samplePos.y > 1) sampleAlpha = 0.0;

            neighborMask += step(targetMin, sampleAlpha) - step(targetMax, sampleAlpha);
            count += 1;
        }
    }
    neighborMask /= count;

    float edgeAmount = centerMask > 0.5 ? 1.0 - neighborMask : neighborMask;
    float outline = smoothstep(0.0, feather, edgeAmount);
    color = mix(color, vec3(1.0, 0.5, 0.062), min(outline, 0.563));

    if (ubo.debugView == debug_Variance) {
        outColor = vec4(visualizeVariance(uv, texSize), 1.0);
        return;
    }

    if (ubo.debugView == debug_SelectionMask) {
        outColor = vec4(visualizeSelectionMask(texelData.a), 1.0);
        return;
    }

    outColor = vec4(color, 1.0);
}
