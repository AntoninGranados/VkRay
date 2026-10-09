#compositing: version(1)

#param float threshold = 0.5: min(0)
#param float feather = 0.01: min(0)
#param float hashingSpacing = 10: min(0)
#param int borderWidth = 1: min(0)
#param vec3 borderColor = vec3(0): color

const mat3 sobelX = mat3(
    -1, 0, 1,
    -2, 0, 2,
    -1, 0, 1
);

const mat3 sobelY = mat3(
    -1, -2, -1,
     0,  0,  0,
     1,  2,  1
);

float getDepth(in PixelInfo info) {
    if (info.aov.skyMask == 0u) return 1 / info.aov.position.z;
    return -1;
}

vec3 getNormal(in PixelInfo info) {
    if (info.aov.skyMask == 0u) return info.aov.normalW;
    return vec3(0);
}

void main() {
    float[2] depthGrad = float[](0, 0);
    vec3[2] normalGrad = vec3[](vec3(0), vec3(0));

    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            ivec2 px = pixelCoord + ivec2(i, j) * borderWidth;
            PixelInfo info = fetchPixelInfoOriginal(px, texSize);

            float wX = sobelX[i+1][j+1];
            float wY = sobelY[i+1][j+1];

            float depth = getDepth(info);
            depthGrad[0] += depth * wX;
            depthGrad[1] += depth * wY;

            vec3 normal = getNormal(info);
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
    vec3 color = sampleSource(pixelCoord, texSize).rgb;
    if (info.aov.skyMask == 1u) result.rgb = color;
    else {
        RngState rng = initRngState(0u);

        vec3 albedo = info.aov.albedo;
        float shadow = luma(color / albedo);
        result.rgb = albedo;
        if (shadow * 3 <= 1) {
            NoiseState2D state = perlinNoise(vec2(pixelCoord) / 8, rng);
            float diag = fract(float(pixelCoord.x + pixelCoord.y) / hashingSpacing);
            result.rgb *= step(0.5 + (state.value - 0.5) * 1, diag);
        }
        if (shadow * 3 <= 2) {
            NoiseState2D state = perlinNoise(vec2(pixelCoord) / 8, rng);
            float diag = fract(float(pixelCoord.x - pixelCoord.y) / hashingSpacing);
            result.rgb *= step(0.5 + (state.value - 0.5) * 1, diag);
        }
        if (shadow > 4) {
            result.rgb = vec3(1);
        }

        result.rgb = mix(result.rgb, borderColor, t);
    }

}
