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

float luma(vec3 c) {
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

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

    // Toon shading
    // PixelInfo pixelInfo = fetchPixelInfoAt(blockCoord, ivec2(texSize));
    // vec3 diffuse = pixelInfo.diffuse.rgb;
    // float lit = clamp(luma(color) / max(luma(diffuse), 1e-4), 0.0, 1.0);
    // const float bands = 4.0;
    // float q = min(floor(lit * bands), bands - 1.0);
    // float toon = q * (1.0 / (bands - 1.0));
    // const float ambiant = 0.01;
    // float targetLuma = luma(diffuse) * (ambiant + (1.0 - ambiant) * toon);
    // color *= targetLuma / max(luma(color), 1e-4);

    // Bloom
    // const float bloomThreshold = 0.7;
    // const float bloomSoftKnee = 0.25;
    // const float bloomStrength = 0.5;
    // const float bloomRadius = 4.0;
    // vec3 bloom = vec3(0.0);
    // float bloomWeight = 0.0;
    // for (int y = -8; y <= 8; y++) {
    //     for (int x = -8; x <= 8; x++) {
    //         vec2 sampleUv = uv + vec2(float(x), float(y)) * texelSize * bloomRadius;
    //         vec3 sampleColor = texture(tex, sampleUv).rgb;
    //         float sampleLuma = luma(sampleColor);
    // 
    //         float xk = sampleLuma - bloomThreshold + bloomSoftKnee;
    //         float soft = clamp(xk, 0.0, 2.0 * bloomSoftKnee);
    //         soft = (soft * soft) / max(4.0 * bloomSoftKnee + 1e-6, 1e-6);
    //         float contrib = max(sampleLuma - bloomThreshold, soft);
    //         vec3 bright = sampleColor * (contrib / max(sampleLuma, 1e-6));
    // 
    //         float dist2 = float(x * x + y * y);
    //         float w = exp(-dist2 / 18.0);
    //         bloom += bright * w;
    //         bloomWeight += w;
    //     }
    // }
    // bloom /= max(bloomWeight, 1e-6);
    // color += bloom * bloomStrength;

    // Outline
    // PixelInfo centerInfo = fetchPixelInfoAt(blockCoord, ivec2(texSize));
    // if (centerInfo.normal.w > 0.0 && centerInfo.position.w > 0.0 && centerInfo.diffuse.w > 0.0) {
    //     int stride = max(int(round(max(ubo.resolution, 1.0))), 1);
    //     ivec2 edgeOffsets[8] = ivec2[](
    //         ivec2(stride, 0),
    //         ivec2(-stride, 0),
    //         ivec2(0, stride),
    //         ivec2(0, -stride),
    //         ivec2(stride * 2, 0),
    //         ivec2(-stride * 2, 0),
    //         ivec2(0, stride * 2),
    //         ivec2(0, -stride * 2)
    //     );
    // 
    //     float normalEdge = 0.0;
    //     float positionEdge = 0.0;
    //     float diffuseEdge = 0.0;
    //     float edgeSampleCount = 0.0;
    //     for (int i = 0; i < 8; i++) {
    //         ivec2 sampleCoord = snapToBlockGrid(blockCoord + edgeOffsets[i], ivec2(texSize));
    //         PixelInfo sampleInfo = fetchPixelInfoAt(sampleCoord, ivec2(texSize));
    //         if (sampleInfo.normal.w <= 0.0 || sampleInfo.position.w <= 0.0 || sampleInfo.diffuse.w <= 0.0) continue;
    // 
    //         normalEdge += length(centerInfo.normal.xyz - sampleInfo.normal.xyz);
    //         positionEdge += length(centerInfo.position.xyz - sampleInfo.position.xyz);
    //         diffuseEdge += length(centerInfo.diffuse.xyz - sampleInfo.diffuse.xyz);
    //         edgeSampleCount += 1.0;
    //     }
    // 
    //     if (edgeSampleCount > 0.0) {
    //         normalEdge /= edgeSampleCount;
    //         positionEdge /= edgeSampleCount;
    //         diffuseEdge /= edgeSampleCount;
    //         float nFactor = smoothstep(0.08, 0.30, normalEdge);
    //         float pFactor = smoothstep(0.01, 0.10, positionEdge);
    //         float dFactor = smoothstep(0.08, 0.35, diffuseEdge);
    //         float edge = max(max(nFactor, pFactor), dFactor);
    //         color *= (1.0 - edge * 0.92);
    //     }
    // }

    outColor = vec4(color, 1.0);
}
