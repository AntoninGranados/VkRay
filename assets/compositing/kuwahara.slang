#compositing: version(1), pass_count(1)

#param float sigma = 2: min(1e-2), max(5)
#param float Q = 8: min(0)

static const int SECTION_COUNT = 8;

int getSection(float angle) {
    float sectionAngle = 2 * PI / SECTION_COUNT;
    return int(mod(round(angle / sectionAngle), SECTION_COUNT));
}

float getGaussianWeight(int2 d, float twoSigmaSq) {
    return exp(- (d.x*d.x + d.y*d.y) / twoSigmaSq);
}

void main() {
    float3 mean[SECTION_COUNT];
    float meanSq[SECTION_COUNT];
    float weight[SECTION_COUNT];
    for (int i = 0; i < SECTION_COUNT; i++) {
        mean[i] = float3(0);
        meanSq[i] = 0;
        weight[i] = 0;
    }

    int radius = int(ceil(3 * sigma));
    float twoSigmaSq = 2 * sigma * sigma;

    for (int dx = -radius; dx <= radius; dx++) {
        for (int dy = -radius; dy <= radius; dy++) {
            int id = getSection(atan2(dy, dx));

            int2 d = int2(dx, dy);
            int2 px = pixelCoord + d;
            px = clamp(px, int2(0), texSize - 1);

            float3 color = sampleOriginal(px, texSize).rgb;
            color = clamp(color, 0, 1);

            float w = getGaussianWeight(d, twoSigmaSq);

            mean[id] += w * color;
            meanSq[id] += w * pow(luma(color), 2);
            weight[id] += w;
        }
    }

    float totalW = 0;
    float3 total = float3(0);
    for (int i = 0; i < SECTION_COUNT; i++) {
        mean[i] /= weight[i];
        meanSq[i] /= weight[i];
        float std = sqrt(max(meanSq[i] - pow(luma(mean[i]), 2), 0));

        float w = 1 / pow(1 + std * 255, Q);
        totalW += w;
        total += mean[i] * w;
    }
    float3 color = total / totalW;

    return float4(color, 1);
}
