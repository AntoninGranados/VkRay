#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

#include "../common.glsl"
#include "../dsl_math_utils.glsl"

layout(set = 0, binding = 0) uniform sampler2D sourceTexture;
layout(set = 0, binding = 1) uniform PassUBO {
    int slot;
    int paramsBase;
    int passId;
} passUbo;
layout(set = 0, binding = 2) buffer PixelInfoBuffer {
    PixelInfo pixels[];
} pixelInfoBuffer;
layout(rgba32f, set = 0, binding = 3) writeonly uniform image2D destinationImage;
layout(set = 0, binding = 4) buffer readonly PluginParamsBuffer {
    float values[];
} pluginParams;
layout(set = 0, binding = 5) uniform sampler2D originalTexture;

PixelInfo fetchPixelInfoOriginal(ivec2 coord, ivec2 texSize) {
    ivec2 c = clamp(coord, ivec2(0), texSize - ivec2(1));
    uint index = uint(c.y * texSize.x + c.x);
    return pixelInfoBuffer.pixels[index];
}

vec3 sampleSource(ivec2 coord, ivec2 texSize) {
    ivec2 c = clamp(coord, ivec2(0), texSize - ivec2(1));
    return texelFetch(sourceTexture, c, 0).rgb;
}

vec3 sampleOriginal(ivec2 coord, ivec2 texSize) {
    ivec2 c = clamp(coord, ivec2(0), texSize - ivec2(1));
    return texelFetch(originalTexture, c, 0).rgb;
}

#include "../generated/compositing_dispatch.glsl"

void main() {
    ivec2 texSize = textureSize(sourceTexture, 0);
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    if (pixelCoord.x >= texSize.x || pixelCoord.y >= texSize.y) return;

    vec3 color = dispatchCompositingPass(passUbo.slot, passUbo.paramsBase, pixelCoord, texSize, passUbo.passId);
    imageStore(destinationImage, pixelCoord, vec4(color, 1.0));
}
