#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

#include "../common.glsl"

layout(set = 0, binding = 0) uniform sampler2D tex;
layout(set = 0, binding = 1) uniform UBO {
    int denoisingEnabled;
} ubo;
layout(set = 0, binding = 2) buffer PixelInfoBuffer {
    PixelInfo pixels[];
} pixelInfoBuffer;
layout(rgba32f, set = 0, binding = 3) writeonly uniform image2D outputTex;

PixelInfo fetchPixelInfoAt(ivec2 coord, ivec2 texSize) {
    ivec2 c = clamp(coord, ivec2(0), texSize - ivec2(1));
    uint index = uint(c.y * texSize.x + c.x);
    return pixelInfoBuffer.pixels[index];
}

vec3 applyATrousDenoise(ivec2 centerBlockCoord, ivec2 texSize) {
    const float c_phi = 0.15;
    const float n_phi = 0.2;
    const float p_phi = 1.0;
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

    int stride = 1;
    vec3 cVal = texelFetch(tex, centerBlockCoord, 0).rgb;
    PixelInfo centerInfo = fetchPixelInfoAt(centerBlockCoord, texSize);

    vec3 sum = vec3(0.0);
    float cumW = 0.0;
    for (int i = 0; i < 25; i++) {
        ivec2 sampleCoord = centerBlockCoord + offsets[i] * stride;
        sampleCoord = clamp(sampleCoord, ivec2(0), texSize - ivec2(1));

        vec3 cTmp = texelFetch(tex, sampleCoord, 0).rgb;
        vec3 cDelta = cVal - cTmp;
        float cDist2 = dot(cDelta, cDelta);
        float cW = min(exp(-cDist2 / max(c_phi, 1e-6)), 1.0);

        PixelInfo sampleInfo = fetchPixelInfoAt(sampleCoord, texSize);

        float nW = 1.0;
        bool nValid = centerInfo.aov.hitValid != 0u && sampleInfo.aov.hitValid != 0u;
        if (nValid) {
            vec3 nDelta = centerInfo.aov.normalW - sampleInfo.aov.normalW;
            float nDist2 = dot(nDelta, nDelta);
            nW = min(exp(-nDist2 / max(n_phi, 1e-6)), 1.0);
        }

        float pW = 1.0;
        bool pValid = centerInfo.aov.hitValid != 0u && sampleInfo.aov.hitValid != 0u;
        if (pValid) {
            vec3 pDelta = centerInfo.aov.positionW - sampleInfo.aov.positionW;
            float pDist2 = dot(pDelta, pDelta) / float(stride * stride);
            pW = min(exp(-pDist2 / max(p_phi, 1e-6)), 1.0);
        }

        float w = kernel[i] * cW * nW * pW;
        sum += cTmp * w;
        cumW += w;
    }

    if (cumW > 1e-8) return sum / cumW;
    return cVal;
}

void main() {
    ivec2 texSize = textureSize(tex, 0);
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);

    if (pixelCoord.x >= texSize.x || pixelCoord.y >= texSize.y) return;

    vec2 screenCoord = vec2(pixelCoord) + vec2(0.5);
    ivec2 blockCoord = pixelCoord;

    vec3 color = texelFetch(tex, blockCoord, 0).rgb;
    if (ubo.denoisingEnabled != 0) {
        color = applyATrousDenoise(blockCoord, texSize);
    }

    imageStore(outputTex, pixelCoord, vec4(color, 1.0));
}
