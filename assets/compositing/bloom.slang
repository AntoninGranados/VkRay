import "compositing_script.slang";

float3 dilate(int2 pixelCoord, int2 texSize, int radius) {
    for (int dx = -radius; dx <= radius; dx++) {
        for (int dy = -radius; dy <= radius; dy++) {
            if (sampleSource(pixelCoord + int2(dx, dy), texSize).r > 0.5)
                return float3(1);
        }
    }
    return float3(0);
}

float3 erode(int2 pixelCoord, int2 texSize, int radius) {
     if (sampleSource(pixelCoord, texSize).r < 0.5) return float3(0);

    for (int dx = -radius; dx <= radius; dx++) {
        for (int dy = -radius; dy <= radius; dy++) {
            if (sampleSource(pixelCoord + int2(dx, dy), texSize).r < 0.5)
                return float3(0);
        }
    }

    return float3(1);
}

float3 blurAlongDir(int2 pixelCoord, int2 texSize, float sigma, int direction) {
    if (sigma < 1e-3) return sampleSource(pixelCoord, texSize).rgb;
    int radius = int(round(3 * sigma));
    float twoSigmaSq = 2 * sigma * sigma;

    float totalW = 0.0;
    float3 total = float3(0);
    for (int dx = -radius; dx <= radius; dx++) {
        int2 dir = direction == 0 ? int2(0, dx) : int2(dx, 0);
        int2 px = pixelCoord + dir;
        if (px.x < 0 || px.x >= texSize.x || px.y < 0 || px.y >= texSize.y) continue;

        float w = exp(- dx * dx / twoSigmaSq);
        totalW += w;
        total += w * sampleSource(px, texSize).rgb;
    }

    return total / totalW;
}

[PassCount(8)]
struct Bloom : ICompositingPass {
    [Min(0)] float intensity = 1;
    [Range(0, 10)] float sigma = 4;
    [Min(0)] float threshold = 0.5;
    [Min(1)] int morphoRadius = 1;

    float4 evaluate(int2 pixelCoord, int2 texSize, int passId) {
        float4 result = sampleSource(pixelCoord, texSize);
        if (passId == 0) { // mask
            float l = luma(result.rgb);
            return float4(float3(l > threshold ? 1 : 0), 1);
        } else if (passId == 1 || passId == 4) {    // erode
            return float4(erode(pixelCoord, texSize, morphoRadius), 1);
        } else if (passId == 2 || passId == 3) {    // dilate
            return float4(dilate(pixelCoord, texSize, morphoRadius), 1);
        } else if (passId == 5 || passId == 6) {    // blur
            return float4(blurAlongDir(pixelCoord, texSize, sigma, passId - 5), 1);
        } else {
            return float4(sampleOriginal(pixelCoord, texSize).rgb + intensity * sampleSource(pixelCoord, texSize).rgb, 1);
        }
    }
}
