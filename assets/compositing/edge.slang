import "compositing_script.slang";

static const float3x3 sobelX = float3x3(
    -1, 0, 1,
    -2, 0, 2,
    -1, 0, 1
);

static const float3x3 sobelY = float3x3(
    -1, -2, -1,
     0,  0,  0,
     1,  2,  1
);

float getDepth(in PixelInfo info) {
    if (info.aov.skyMask == 0u) return 1 / info.aov.position.z;
    return -1;
}

float3 getNormal(in PixelInfo info) {
    if (info.aov.skyMask == 0u) return info.aov.normalW;
    return float3(0);
}

struct Edge : ICompositingPass {
    [Min(0)] float threshold = 0.5;
    [Min(0)] float feather = 0.01;
    [Min(0)] float hashingSpacing = 10;
    [Min(0)] int borderWidth = 1;
    [Color] float3 borderColor = float3(0);

    float4 evaluate(int2 pixelCoord, int2 texSize, int passId) {
        float4 result = sampleSource(pixelCoord, texSize);
        float depthGrad[2] = { 0, 0 };
        float3 normalGrad[2] = { float3(0), float3(0) };

        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                int2 px = pixelCoord + int2(i, j) * borderWidth;
                PixelInfo info = fetchPixelInfoOriginal(px, texSize);

                float wX = sobelX[i+1][j+1];
                float wY = sobelY[i+1][j+1];

                float depth = getDepth(info);
                depthGrad[0] += depth * wX;
                depthGrad[1] += depth * wY;

                float3 normal = getNormal(info);
                normalGrad[0] += normal * wX;
                normalGrad[1] += normal * wY;
            }
        }

        float dx, dy;
        float grad;

        dx = depthGrad[0]; dy = depthGrad[1];
        grad = sqrt(dx*dx + dy*dy);

        dx = length(normalGrad[0]); dy = length(normalGrad[1]);
        grad += sqrt(dx*dx + dy*dy);

        float t = smoothstep(-feather, feather, grad - threshold);

        PixelInfo info = fetchPixelInfoOriginal(pixelCoord, texSize);
        float3 color = sampleSource(pixelCoord, texSize).rgb;
        if (info.aov.skyMask == 1u) result.rgb = color;
        else {
            RngState rng = initRngState(0u);

            float3 albedo = info.aov.albedo;
            float shadow = luma(color / albedo);
            result.rgb = albedo;
            if (shadow * 3 <= 1) {
                NoiseState2D state = perlinNoise(float2(pixelCoord) / 8, rng);
                float diag = frac(float(pixelCoord.x + pixelCoord.y) / hashingSpacing);
                result.rgb *= step(0.5 + (state.value - 0.5) * 1, diag);
            }
            if (shadow * 3 <= 2) {
                NoiseState2D state = perlinNoise(float2(pixelCoord) / 8, rng);
                float diag = frac(float(pixelCoord.x - pixelCoord.y) / hashingSpacing);
                result.rgb *= step(0.5 + (state.value - 0.5) * 1, diag);
            }
            if (shadow > 4) {
                result.rgb = float3(1);
            }

            result.rgb = lerp(result.rgb, borderColor, t);
        }

        return result;
    }
}
