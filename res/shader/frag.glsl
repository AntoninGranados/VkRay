#version 450

layout(set = 0, binding = 0) uniform sampler2D tex;
// layout(set = 0, binding = 1) uniform UBO {
// } ubo;

layout(location = 0) in vec2 fragPos;
layout(location = 0) out vec4 outColor;

const float outlineWidth = 3.0;
const float feather = 0.8;

void main() {
    // TODO fix the edge issue (edges wrap around the screen)
    vec2 uv = fragPos * 0.5 + 0.5;

    vec4 texelData = texture(tex, uv);
    vec3 color = texelData.rgb;

    vec2 texelSize = 1.0 / vec2(textureSize(tex, 0));

    float targetMin = 0.5;
    float targetMax = 1.5;

    float centerAlpha = texelData.a;
    float centerMask = step(targetMin, centerAlpha) - step(targetMax, centerAlpha);
    vec2 stepV = texelSize * outlineWidth;

    float neighborMask = 0.0;
    int count = 0;
    vec2 samplePos;
    float sampleAlpha;
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            if (i == 0 && j == 0) continue;
            
            samplePos = uv + vec2(i, j) * stepV;
            sampleAlpha = texture(tex, samplePos).a;
            if (0 > samplePos.x || samplePos.x > 1) sampleAlpha = 0.0;
            if (0 > samplePos.y || samplePos.y > 1) sampleAlpha = 0.0;

            neighborMask += step(targetMin, sampleAlpha) - step(targetMax, sampleAlpha);
            count += 1;
        }
    }
    neighborMask /= count;

    float edgeAmount = centerMask > 0.5 ? 1.0 - neighborMask : neighborMask;
    float outline = smoothstep(0.0, feather, edgeAmount);
    color = mix(color, vec3(1.0, 0.5, 0.062), min(outline, 0.563));

    outColor = vec4(color, 1.0);
}
