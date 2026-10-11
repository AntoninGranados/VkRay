import "utils.slang";

uint varianceIndexFromCoord(int2 coord, int2 texSize) {
    return uint(coord.y * texSize.x + coord.x);
}

void updateVariance(in float value, inout PixelInfo pixelInfo) {
    float delta = value - pixelInfo.mean;
    pixelInfo.mean += delta / pixelInfo.count;
    float delta2 = value - pixelInfo.mean;
    pixelInfo.m2 += delta * delta2;
}

float computeSpatialVariance(Sampler2D previousFrame, int2 blockCoord, float2 texSize) {
    const int radius = 2;
    float mean = 0.0;
    float meanSq = 0.0;
    int samples = 0;
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            int2 p = blockCoord + int2(x, y);
            p = clamp(p, int2(0), int2(texSize) - int2(1));
            float3 c = previousFrame.Load(int3(p, 0)).rgb;
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

float computeSampleProbability(inout PixelInfo pixelInfo, Sampler2D previousFrame, int warmupSamples, int2 blockCoord, int2 texSize) {
    float temporalVariance = (pixelInfo.count > 1.0) ? (pixelInfo.m2 / (pixelInfo.count - 1.0)) : 0.0;
    float spatialSigma = computeSpatialVariance(previousFrame, blockCoord, texSize);

    float sigma = lerp(sqrt(max(temporalVariance, 0.0)), spatialSigma, 0.5);
    float minAdaptiveSamples = max(float(warmupSamples), 0.0);
    float proba = clamp(sigma * 4.0, 0.1, 1.0);
    pixelInfo.varianceProba = proba;
    return (pixelInfo.count < minAdaptiveSamples) ? 1.0 : proba;
}
