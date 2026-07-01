#ifndef ADAPTIVE_SAMPLING_GLSL
#define ADAPTIVE_SAMPLING_GLSL

#include "inputs.glsl"
#include "utils.glsl"
#include "random.glsl"

uint varianceIndexFromCoord(ivec2 coord, ivec2 texSize) {
    return uint(coord.y * texSize.x + coord.x);
}

ivec2 blockCoordFromResolution(ivec2 pixelCoord, vec2 screenCoord, ivec2 texSize, float resolution) {
    ivec2 blockCoord = pixelCoord;
    if (resolution > 1.0f) {
        blockCoord = ivec2(floor(screenCoord / resolution) * resolution);
    }
    return clamp(blockCoord, ivec2(0), texSize - ivec2(1));
}

void updateVariance(in float value, inout PixelInfo pixelInfo) {
    float delta = value - pixelInfo.mean;
    pixelInfo.mean += delta / pixelInfo.count;
    float delta2 = value - pixelInfo.mean;
    pixelInfo.m2 += delta * delta2;
}

float computeSpatialVariance(ivec2 blockCoord, vec2 texSize) {
    const int radius = 2;
    float mean = 0.0;
    float meanSq = 0.0;
    int samples = 0;
    float stepSize = max(ubo.screen.resolution, 1.0);
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            ivec2 p = blockCoord + ivec2(x, y) * int(stepSize);
            p = clamp(p, ivec2(0), ivec2(texSize) - ivec2(1));
            vec3 c = texelFetch(prevTex, p, 0).rgb;
            float lum = luma(c);
            mean += lum;
            meanSq += lum * lum;
            samples++;
        }
    }
    mean /= float(samples);
    meanSq /= float(samples);
    return sqrt(max(meanSq - mean * mean, 0.0));
}

float computeSampleProbability(inout PixelInfo pixelInfo, ivec2 blockCoord, ivec2 texSize, float resolution) {
    float temporalVariance = (pixelInfo.count > 1.0) ? (pixelInfo.m2 / (pixelInfo.count - 1.0)) : 0.0;
    float spatialSigma = computeSpatialVariance(blockCoord, texSize);

    float sigma = mix(sqrt(max(temporalVariance, 0.0)), spatialSigma, 0.5);
    float minAdaptiveSamples = max(float(ubo.render.varianceWarmupSamples), 0.0);
    float proba = clamp(sigma * 4.0, 0.1, 1.0);
    pixelInfo.varianceProba = proba;
    return (pixelInfo.count < minAdaptiveSamples) ? 1.0 : proba;
}

#endif
