// --------------- INPUTS ---------------
// ivec2 pixelCoord
// ivec2 texSize
// int passId
// vec3 sampleSource(ivec2 coord, ivec2 texSize)
// vec3 sampleOriginal(ivec2 coord, ivec2 texSize)
// PixelInfo fetchPixelInfoOriginal(ivec2 coord, ivec2 texSize)

// --------------- OUTPUTS ---------------
// vec3 result

#compositing: version(1), pass_count(5)

#param float cPhi = 0.05: min(0)
#param float nPhi = 0.05: min(0)
#param float pPhi = 1.0: min(0)

void main() {
    int stride = 1 << passId;

    const float kernel[25] = float[](
        1.0/256.0, 1.0/64.0, 3.0/128.0, 1.0/64.0, 1.0/256.0,
        1.0/64.0,  1.0/16.0, 3.0/32.0,  1.0/16.0, 1.0/64.0,
        3.0/128.0, 3.0/32.0, 9.0/64.0,  3.0/32.0, 3.0/128.0,
        1.0/64.0,  1.0/16.0, 3.0/32.0,  1.0/16.0, 1.0/64.0,
        1.0/256.0, 1.0/64.0, 3.0/128.0, 1.0/64.0, 1.0/256.0
    );
    const ivec2 offsets[25] = ivec2[](
        ivec2(-2,-2), ivec2(-1,-2), ivec2( 0,-2), ivec2( 1,-2), ivec2( 2,-2),
        ivec2(-2,-1), ivec2(-1,-1), ivec2( 0,-1), ivec2( 1,-1), ivec2( 2,-1),
        ivec2(-2, 0), ivec2(-1, 0), ivec2( 0, 0), ivec2( 1, 0), ivec2( 2, 0),
        ivec2(-2, 1), ivec2(-1, 1), ivec2( 0, 1), ivec2( 1, 1), ivec2( 2, 1),
        ivec2(-2, 2), ivec2(-1, 2), ivec2( 0, 2), ivec2( 1, 2), ivec2( 2, 2)
    );

    vec3 cVal = sampleSource(pixelCoord, texSize);
    PixelInfo centerInfo = fetchPixelInfoOriginal(pixelCoord, texSize);

    float cPhiPass = cPhi / float(1 << passId);

    vec3 sum = vec3(0.0);
    float cumW = 0.0;
    for (int i = 0; i < 25; i++) {
        ivec2 sampleCoord = pixelCoord + offsets[i] * stride;

        vec3 cTmp = sampleSource(sampleCoord, texSize);
        float cW = 1.0;
        if (passId > 0) {
            vec3 cDelta = cVal - cTmp;
            float cDist2 = dot(cDelta, cDelta);
            cW = min(exp(-cDist2 / max(cPhiPass, 1e-6)), 1.0);
        }

        PixelInfo sampleInfo = fetchPixelInfoOriginal(sampleCoord, texSize);

        float nW = 1.0;
        float pW = 1.0;
        bool centerHit = centerInfo.aov.hitValid != 0u;
        bool sampleHit = sampleInfo.aov.hitValid != 0u;
        if (centerHit != sampleHit) {
            nW = 0.0;
            pW = 0.0;
        } else if (centerHit && sampleHit) {
            vec3 nDelta = centerInfo.aov.normalW - sampleInfo.aov.normalW;
            float nDist2 = dot(nDelta, nDelta);
            nW = min(exp(-nDist2 / max(nPhi, 1e-6)), 1.0);

            vec3 pDelta = centerInfo.aov.positionW - sampleInfo.aov.positionW;
            float pDist2 = dot(pDelta, pDelta) / float(stride * stride);
            pW = min(exp(-pDist2 / max(pPhi, 1e-6)), 1.0);
        }

        float w = kernel[i] * cW * nW * pW;
        sum += cTmp * w;
        cumW += w;
    }

    if (cumW > 1e-8) result = sum / cumW;
    else result = cVal;
}
