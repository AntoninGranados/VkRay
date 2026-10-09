#compositing: version(1), pass_count(4)

#param float sigma1 = 2.0: min(0)
#param float sigma2 = 3.0: min(0)
#param float threshold = 0.5: min(0)

vec2 blurAlongDir(ivec2 pixelCoord, ivec2 texSize, float sigma1, float sigma2, int direction) {
    if (sigma1 < 1e-3 && sigma2 < 1e-3) return sampleSource(pixelCoord, texSize).rg;

    int radius = int(round(3 * max(sigma1, sigma2)));
    float twoSigma1Sq = 2 * sigma1 * sigma1;
    float twoSigma2Sq = 2 * sigma2 * sigma2;

    vec2 totalW = vec2(0);
    vec2 total = vec2(0);

    float c, w;
    for (int dx = -radius; dx <= radius; dx++) {
        ivec2 dir = direction == 0 ? ivec2(0, dx) : ivec2(dx, 0);
        ivec2 px = pixelCoord + dir;
        if (px.x < 0 || px.x >= texSize.x || px.y < 0 || px.y >= texSize.y) continue;

        vec3 color = sampleSource(px, texSize).rgb;
        c = color.r;
        w = exp(- dx * dx / twoSigma1Sq);
        totalW[0] += w;
        total[0] += w * c;

        c = color.g;
        w = exp(- dx * dx / twoSigma2Sq);
        totalW[1] += w;
        total[1] += w * c;
    }

    return total / totalW;
}

void main() {
    switch (passId) {
        case 0: return vec4(vec3(luma(sampleOriginal(pixelCoord, texSize).rgb)), 1);
        case 1:
        case 2: return vec4(blurAlongDir(pixelCoord, texSize, sigma1, sigma2, passId-1), 0, 1);
        case 3: return vec4(vec3(abs(result.g - result.r)), 1); // vec3(abs(result.g - result.r) > threshold ? 1 : 0);
    }
}
