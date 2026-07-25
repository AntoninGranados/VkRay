#version 450

#include "../common.glsl"

layout(set = 0, binding = 0) uniform sampler2D outputTex;
layout(set = 0, binding = 1) buffer PixelInfoBuffer {
    PixelInfo pixels[];
} pixelInfoBuffer;
layout(set = 0, binding = 2, rgba32f) uniform writeonly image2D displayOut;

layout(local_size_x = 8, local_size_y = 8) in;

const float outlineWidth = 2.0;
const float feather      = 0.4;
const vec3  edgeColor    = vec3(1.0, 0.5, 0.062);

void main() {
    ivec2 coord   = ivec2(gl_GlobalInvocationID.xy);
    ivec2 texSize = textureSize(outputTex, 0);
    if (coord.x >= texSize.x || coord.y >= texSize.y) return;

    vec3 color = texelFetch(outputTex, coord, 0).rgb;

    uint pixelIndex = uint(coord.y * texSize.x + coord.x);
    PixelInfo blockPixelInfo = pixelInfoBuffer.pixels[pixelIndex];
    uint centerMask = blockPixelInfo.selectionMask;
    int stepPx = int(outlineWidth);

    float neighborMask = 0.0;
    int count = 0;
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            if (i == 0 && j == 0) continue;

            ivec2 sampleCoord = coord + ivec2(i, j) * stepPx;
            if (sampleCoord.x < 0 || sampleCoord.x >= texSize.x) continue;
            if (sampleCoord.y < 0 || sampleCoord.y >= texSize.y) continue;

            uint sampleIndex = uint(sampleCoord.y * texSize.x + sampleCoord.x);
            neighborMask += pixelInfoBuffer.pixels[sampleIndex].selectionMask != 0u ? 1.0 : 0.0;
            count += 1;
        }
    }
    neighborMask /= count;

    float edgeAmount = centerMask != 0u ? 1.0 - neighborMask : neighborMask;
    color = mix(color, edgeColor, smoothstep(0.0, feather, edgeAmount));

    imageStore(displayOut, coord, vec4(color, 1.0));
}
