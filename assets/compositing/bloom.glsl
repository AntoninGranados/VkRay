#compositing: version(1), pass_count(8)

#param float intensity = 1: min(0)
#param float sigma = 4: min(0), max(10)
#param float threshold = 0.5: min(0)
#param int morphoRadius = 1: min(1)

vec3 dilate(ivec2 pixelCoord, ivec2 texSize, int radius) {
    for (int dx = -radius; dx <= radius; dx++) {
        for (int dy = -radius; dy <= radius; dy++) {
            if (sampleSource(pixelCoord + ivec2(dx, dy), texSize).r > 0.5)
                return vec3(1);
        }
    }
    return vec3(0);
}

vec3 erode(ivec2 pixelCoord, ivec2 texSize, int radius) {
     if (sampleSource(pixelCoord, texSize).r < 0.5) return vec3(0);

    for (int dx = -radius; dx <= radius; dx++) {
        for (int dy = -radius; dy <= radius; dy++) {
            if (sampleSource(pixelCoord + ivec2(dx, dy), texSize).r < 0.5)
                return vec3(0);
        }
    }

    return vec3(1);
}

vec3 blurAlongDir(ivec2 pixelCoord, ivec2 texSize, float sigma, int direction) {
    if (sigma < 1e-3) return sampleSource(pixelCoord, texSize);
    int radius = int(round(3 * sigma));
    float twoSigmaSq = 2 * sigma * sigma;

    float totalW = 0.0;
    vec3 total = vec3(0);
    for (int dx = -radius; dx <= radius; dx++) {
        ivec2 dir = direction == 0 ? ivec2(0, dx) : ivec2(dx, 0);
        ivec2 px = pixelCoord + dir;
        if (px.x < 0 || px.x >= texSize.x || px.y < 0 || px.y >= texSize.y) continue;

        float w = exp(- dx * dx / twoSigmaSq);
        totalW += w;
        total += w * sampleSource(px, texSize);
    }

    return total / totalW;
}

void main() {
    if (passId == 0) { // mask
        float l = luma(result);
        return vec3(l > threshold ? 1 : 0);
    } else if (passId == 1 || passId == 4) {    // erode
        return erode(pixelCoord, texSize, morphoRadius);
    } else if (passId == 2 || passId == 3) {    // dilate
        return dilate(pixelCoord, texSize, morphoRadius);
    } else if (passId == 5 || passId == 6) {    // blur
        return blurAlongDir(pixelCoord, texSize, sigma, passId - 5);
    } else {
        return sampleOriginal(pixelCoord, texSize) + intensity * sampleSource(pixelCoord, texSize);
    }
}
