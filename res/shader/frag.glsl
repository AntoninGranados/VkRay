#version 450

layout(set = 0, binding = 0) uniform sampler2D tex;
layout(set = 0, binding = 1) uniform UBO {
    int sphereCount;
    int sphereId;
} ubo;

layout(location = 0) in vec2 fragPos;
layout(location = 0) out vec4 outColor;

float hash13(vec3 p3) {
    p3  = fract(p3 * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float rand(inout vec3 seed) {
    float r = hash13(seed);
    seed += vec3(1.0, 1.0, 1.0);
    return r;
}

void main() {
    vec2 uv = fragPos * 0.5 + 0.5;

    vec4 texelData = texture(tex, uv);
    vec3 color = texelData.rgb;

    if (ubo.sphereId > 0) {
        vec2 texelSize = 1.0 / vec2(textureSize(tex, 0));
        const float outlineWidth = 3.0;
        const float feather = 0.2;

        float targetMin = (float(ubo.sphereId) - 0.5) / float(ubo.sphereCount);
        float targetMax = (float(ubo.sphereId) + 0.5) / float(ubo.sphereCount);

        float centerAlpha = texelData.a;
        float centerMask = step(targetMin, centerAlpha) - step(targetMax, centerAlpha);
        vec2 stepV = texelSize * outlineWidth;

        float neighborMask = 0.0;
        float sampleAlpha;
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                if (i == 0 && j == 0) continue;
                sampleAlpha = texture(tex, uv + vec2(i, j) * stepV).a;
                neighborMask += step(targetMin, sampleAlpha) - step(targetMax, sampleAlpha);
            }
        }
        neighborMask *= 0.125;

        float edgeAmount = centerMask > 0.5 ? 1.0 - neighborMask : neighborMask;
        float outline = smoothstep(0.0, feather, edgeAmount);
        color = mix(color, vec3(1.0, 0.5, 0.062), min(outline, 0.563));
    }

    outColor = vec4(color, 1.0);
}
