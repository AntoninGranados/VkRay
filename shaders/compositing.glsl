#version 450

struct PixelInfo {
    vec4 normal;
    vec4 position;
    vec4 diffuse;
    float mean;
    float m2;
    int count;
    float varianceProba;
    int selectionMask;
};

layout(set = 0, binding = 0) uniform sampler2D tex;
layout(set = 0, binding = 1) uniform UBO {
    int frameCount;
    float resolution;
    int debugView;
    int previewBorderEnabled;
    int denoisingEnabled;
} ubo;
layout(set = 0, binding = 2) buffer PixelInfoBuffer {
    PixelInfo pixels[];
} pixelInfoBuffer;

layout(location = 0) in vec2 fragPos;
layout(location = 0) out vec4 outColor;


ivec2 blockCoordFromResolution(ivec2 pixelCoord, vec2 screenCoord, ivec2 texSize, float resolution) {
    ivec2 blockCoord = pixelCoord;
    if (resolution > 1.0) {
        blockCoord = ivec2(floor(screenCoord / resolution) * resolution);
    }
    return clamp(blockCoord, ivec2(0), texSize - ivec2(1));
}

ivec2 snapToBlockGrid(ivec2 coord, ivec2 texSize) {
    float res = max(ubo.resolution, 1.0);
    ivec2 block = ivec2(floor(vec2(coord) / res) * res);
    return clamp(block, ivec2(0), texSize - ivec2(1));
}

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

    int stride = max(int(round(max(ubo.resolution, 1.0))), 1);
    vec3 cVal = texelFetch(tex, centerBlockCoord, 0).rgb;
    PixelInfo centerInfo = fetchPixelInfoAt(centerBlockCoord, texSize);

    vec3 sum = vec3(0.0);
    float cumW = 0.0;
    for (int i = 0; i < 25; i++) {
        ivec2 sampleCoord = centerBlockCoord + offsets[i] * stride;
        sampleCoord = snapToBlockGrid(sampleCoord, texSize);

        vec3 cTmp = texelFetch(tex, sampleCoord, 0).rgb;
        vec3 cDelta = cVal - cTmp;
        float cDist2 = dot(cDelta, cDelta);
        float cW = min(exp(-cDist2 / max(c_phi, 1e-6)), 1.0);

        PixelInfo sampleInfo = fetchPixelInfoAt(sampleCoord, texSize);

        float nW = 1.0;
        bool nValid = centerInfo.normal.w > 0.0 && sampleInfo.normal.w > 0.0;
        if (nValid) {
            vec3 nDelta = centerInfo.normal.xyz - sampleInfo.normal.xyz;
            float nDist2 = dot(nDelta, nDelta);
            nW = min(exp(-nDist2 / max(n_phi, 1e-6)), 1.0);
        }

        float pW = 1.0;
        bool pValid = centerInfo.position.w > 0.0 && sampleInfo.position.w > 0.0;
        if (pValid) {
            vec3 pDelta = centerInfo.position.xyz - sampleInfo.position.xyz;
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
    vec2 uv = fragPos * 0.5 + 0.5;

    vec2 texSize = vec2(textureSize(tex, 0));
    vec2 texelSize = 1.0 / texSize;

    vec2 screenCoord = uv * texSize;
    ivec2 pixelCoord = ivec2(screenCoord);
    ivec2 blockCoord = blockCoordFromResolution(pixelCoord, screenCoord, ivec2(texSize), ubo.resolution);
    vec3 color = texelFetch(tex, blockCoord, 0).rgb;
    if (ubo.denoisingEnabled != 0) {
        color = applyATrousDenoise(blockCoord, ivec2(texSize));
    }

    outColor = vec4(color, 1.0);
}
