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

const float outlineWidth = 2.0;
const float feather = 0.4;
#define debug_None          0
#define debug_Bounces       1
#define debug_Normal        2
#define debug_Position      3
#define debug_Diffuse       4
#define debug_SelectionMask 5
#define debug_Variance      6

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

vec3 visualizeVariance(vec2 uv, vec2 texSize) {
    vec2 screenCoord = uv * texSize;
    ivec2 pixelCoord = ivec2(screenCoord);
    ivec2 blockCoord = blockCoordFromResolution(pixelCoord, screenCoord, ivec2(texSize), ubo.resolution);
    uint index = uint(blockCoord.y * int(texSize.x) + blockCoord.x);
    PixelInfo pixelInfo = pixelInfoBuffer.pixels[index];
    
    return vec3(pixelInfo.varianceProba);
}

vec3 visualizeSelectionMask(int mask) {
    return vec3(mask != 0 ? 1.0 : 0.0);
}

vec3 visualizeNormal(PixelInfo pixelInfo) {
    if (pixelInfo.normal.w <= 0.0) return vec3(0.0);
    return normalize(pixelInfo.normal.xyz) * 0.5 + 0.5;
}

vec3 visualizePosition(PixelInfo pixelInfo) {
    if (pixelInfo.position.w <= 0.0) return vec3(0.0);
    return clamp(pixelInfo.position.xyz, 0.0, 1.0);
}

vec3 visualizeDiffuse(PixelInfo pixelInfo) {
    if (pixelInfo.diffuse.w <= 0.0) return vec3(0.0);
    return clamp(pixelInfo.diffuse.xyz, 0.0, 1.0);
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
    const float d_phi = 1.0;
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

        float dW = 1.0;
        bool dValid = centerInfo.diffuse.w > 0.0 && sampleInfo.diffuse.w > 0.0;
        if (dValid) {
            vec3 dDelta = centerInfo.diffuse.xyz - sampleInfo.diffuse.xyz;
            float dDist2 = dot(dDelta, dDelta);
            dW = min(exp(-dDist2 / max(d_phi, 1e-6)), 1.0);
        }

        float w = kernel[i] * cW * nW * pW * dW;
        sum += cTmp * w;
        cumW += w;
    }

    if (cumW > 1e-8) return sum / cumW;
    return cVal;
}

const vec3 edgeColor = vec3(1.0, 0.5, 0.062);

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
    uint blockIndex = uint(blockCoord.y * int(texSize.x) + blockCoord.x);

    float targetMin = 0.5;
    float targetMax = 1.5;

    uint centerIndex = uint(pixelCoord.y * int(texSize.x) + pixelCoord.x);
    PixelInfo centerPixelInfo = pixelInfoBuffer.pixels[centerIndex];
    int centerMask = centerPixelInfo.selectionMask;
    int stepPx = int(outlineWidth);

    float neighborMask = 0.0;
    int count = 0;
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            if (i == 0 && j == 0) continue;

            ivec2 sampleCoord = pixelCoord + ivec2(i, j) * stepPx;
            if (sampleCoord.x < 0 || sampleCoord.x >= int(texSize.x)) continue;
            if (sampleCoord.y < 0 || sampleCoord.y >= int(texSize.y)) continue;

            uint sampleIndex = uint(sampleCoord.y * int(texSize.x) + sampleCoord.x);
            neighborMask += pixelInfoBuffer.pixels[sampleIndex].selectionMask != 0 ? 1.0 : 0.0;
            count += 1;
        }
    }
    neighborMask /= count;

    float edgeAmount = centerMask != 0 ? 1.0 - neighborMask : neighborMask;
    float outline = smoothstep(0.0, feather, edgeAmount);
    // color = mix(color, edgeColor, min(outline, 0.5));
    color = mix(color, edgeColor, outline);

    if (ubo.debugView == debug_Variance) {
        outColor = vec4(visualizeVariance(uv, texSize), 1.0);
        return;
    }

    if (ubo.debugView == debug_Normal) {
        outColor = vec4(visualizeNormal(centerPixelInfo), 1.0);
        return;
    }

    if (ubo.debugView == debug_Position) {
        outColor = vec4(visualizePosition(centerPixelInfo), 1.0);
        return;
    }

    if (ubo.debugView == debug_Diffuse) {
        outColor = vec4(visualizeDiffuse(centerPixelInfo), 1.0);
        return;
    }

    if (ubo.debugView == debug_SelectionMask) {
        outColor = vec4(visualizeSelectionMask(centerMask), 1.0);
        return;
    }

    if (ubo.previewBorderEnabled != 0) {
        if (abs(fragPos.x) > 0.8 || abs(fragPos.y) > 0.8) {
            color *= 0.2;
        }
        if ((abs(fragPos.x) < 0.8 + 2*outlineWidth/texSize.x && abs(fragPos.x) > 0.8 - 2*outlineWidth/texSize.x) && abs(fragPos.y) < 0.8) {
            color = edgeColor;
        }
        if ((abs(fragPos.y) < 0.8 + 2*outlineWidth/texSize.y && abs(fragPos.y) > 0.8 - 2*outlineWidth/texSize.y) && abs(fragPos.x) < 0.8) {
            color = edgeColor;
        }
    }

    outColor = vec4(color, 1.0);
}
