// --------------- INPUTS ---------------
// int2 pixelCoord
// int2 texSize
// int passId
// float4 sampleSource(int2 coord, int2 texSize)
// float4 sampleOriginal(int2 coord, int2 texSize)
// PixelInfo fetchPixelInfoOriginal(int2 coord, int2 texSize)

// --------------- OUTPUTS ---------------
// float4 (return value)

import "compositing_script.slang";

[PassCount(5)]
struct ATrous : ICompositingPass {
    [Min(0)] float cPhi = 0.05;
    [Min(0)] float nPhi = 0.05;
    [Min(0)] float pPhi = 1.0;

    float4 evaluate(int2 pixelCoord, int2 texSize, int passId) {
        float4 result = sampleSource(pixelCoord, texSize);
        int stride = 1 << passId;

        const float kernel[25] = {
            1.0/256.0, 1.0/64.0, 3.0/128.0, 1.0/64.0, 1.0/256.0,
            1.0/64.0,  1.0/16.0, 3.0/32.0,  1.0/16.0, 1.0/64.0,
            3.0/128.0, 3.0/32.0, 9.0/64.0,  3.0/32.0, 3.0/128.0,
            1.0/64.0,  1.0/16.0, 3.0/32.0,  1.0/16.0, 1.0/64.0,
            1.0/256.0, 1.0/64.0, 3.0/128.0, 1.0/64.0, 1.0/256.0
        };
        const int2 offsets[25] = {
            int2(-2,-2), int2(-1,-2), int2( 0,-2), int2( 1,-2), int2( 2,-2),
            int2(-2,-1), int2(-1,-1), int2( 0,-1), int2( 1,-1), int2( 2,-1),
            int2(-2, 0), int2(-1, 0), int2( 0, 0), int2( 1, 0), int2( 2, 0),
            int2(-2, 1), int2(-1, 1), int2( 0, 1), int2( 1, 1), int2( 2, 1),
            int2(-2, 2), int2(-1, 2), int2( 0, 2), int2( 1, 2), int2( 2, 2)
        };

        float3 cVal = sampleSource(pixelCoord, texSize).rgb;
        PixelInfo centerInfo = fetchPixelInfoOriginal(pixelCoord, texSize);

        float cPhiPass = cPhi / float(1 << passId);

        float3 sum = float3(0.0);
        float cumW = 0.0;
        for (int i = 0; i < 25; i++) {
            int2 sampleCoord = pixelCoord + offsets[i] * stride;

            float3 cTmp = sampleSource(sampleCoord, texSize).rgb;
            float cW = 1.0;
            if (passId > 0) {
                float3 cDelta = cVal - cTmp;
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
                float3 nDelta = centerInfo.aov.normalW - sampleInfo.aov.normalW;
                float nDist2 = dot(nDelta, nDelta);
                nW = min(exp(-nDist2 / max(nPhi, 1e-6)), 1.0);

                float3 pDelta = centerInfo.aov.positionW - sampleInfo.aov.positionW;
                float pDist2 = dot(pDelta, pDelta) / float(stride * stride);
                pW = min(exp(-pDist2 / max(pPhi, 1e-6)), 1.0);
            }

            float w = kernel[i] * cW * nW * pW;
            sum += cTmp * w;
            cumW += w;
        }

        if (cumW > 1e-8) result.rgb = sum / cumW;
        else result.rgb = cVal;
        return result;
    }
}
