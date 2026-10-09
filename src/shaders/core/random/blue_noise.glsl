#ifndef RANDOM_BLUE_NOISE_GLSL
#define RANDOM_BLUE_NOISE_GLSL

#include "basics.glsl"
#include "../inputs.glsl"

#define BLUE_NOISE_SIZE 128u
#define BLUE_NOISE_SLICES 64u

float blueNoise(uvec2 pixel, uint dimension) {
    uvec2 tile = pixel / BLUE_NOISE_SIZE;
    uvec2 texel = pixel % BLUE_NOISE_SIZE;
    uint orientation = pcgHash(tile.x + tile.y * 4096u + dimension * 1315423911u) >> 29u;
    if ((orientation & 1u) != 0u) texel.x = BLUE_NOISE_SIZE - 1u - texel.x;
    if ((orientation & 2u) != 0u) texel.y = BLUE_NOISE_SIZE - 1u - texel.y;
    if ((orientation & 4u) != 0u) texel = texel.yx;
    uint index = ((dimension % BLUE_NOISE_SLICES) * BLUE_NOISE_SIZE + texel.y) * BLUE_NOISE_SIZE + texel.x;
    uint value = (blueNoiseBuffer.values[index >> 1u] >> ((index & 1u) * 16u)) & 0xFFFFu;
    return (float(value) + 0.5) / 65536.0;
}

#endif
