import "../core/scene.slang";
import "../core/global.slang";
import "../core/random/utils.slang";
import "../core/camera/camera.slang";

struct DisplayUBO {
    int  showFocusPlane;
    int  selectedObjectId;
    int  previewBorderEnabled;
    float2 previewFrameExtent;
    float4 focusPlane;
    CameraUBO camera;
};

ConstantBuffer<DisplayUBO> ubo;
Sampler2D outputTex;
RWStructuredBuffer<PixelInfo> pixelInfoBuffer;
[[vk::image_format("rgba32f")]] RWTexture2D<float4> displayOut;

static const float outlineWidth = 3.0;
static const float previewStripeSpacing = 64.0;
static const float feather      = 0.4;
static const float3  edgeColor    = float3(1.0, 0.5, 0.062);
static const float3  focusColor   = float3(1.0, 0.3, 1.0);

PixelInfo samplePixelInfo(int2 vpCoord, int2 vpSize, int2 renderSize) {
    int2 rc = clamp(vpCoord * renderSize / vpSize, int2(0), renderSize - int2(1));
    return pixelInfoBuffer[uint(rc.y * renderSize.x + rc.x)];
}

Ray viewportRay(int2 coord, int2 viewportSize) {
    float2 ndc = (float2(coord) + 0.5) / float2(viewportSize) * 2.0 - 1.0;
    RngState rng = initRngState(uint2(coord), 0u);
    return getRay(ubo.camera, ndc, rng);
}

[shader("compute")]
[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID) {
    int2 coord        = int2(dispatchThreadId.xy);
    int2 viewportSize = imageSize(displayOut);
    if (coord.x >= viewportSize.x || coord.y >= viewportSize.y) return;

    int2 renderSize  = textureSize(outputTex);
    int2 renderCoord = clamp(coord * renderSize / viewportSize, int2(0), renderSize - int2(1));

    float3 color = outputTex.Load(int3(renderCoord, 0)).rgb;

    bool isSelected = false;
    if (ubo.selectedObjectId >= 0) {
        Ray selRay = viewportRay(coord, viewportSize);
        Statistics dummy = Statistics(0u, 0u);
        Hit selHit = rayObjectIntersection(selRay, loadObject(uint(ubo.selectedObjectId)), false, INFINITY, dummy);
        isSelected = foundIntersection(selHit);
    }

    uint centerMask = isSelected ? 1u : 0u;
    int stepPx = int(outlineWidth);
    float neighborMask = 0.0;
    int count = 0;
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            if (i == 0 && j == 0) continue;
            int2 nb = coord + int2(i, j) * stepPx;
            if (nb.x < 0 || nb.x >= viewportSize.x || nb.y < 0 || nb.y >= viewportSize.y) continue;
            if (ubo.selectedObjectId >= 0) {
                Ray nbRay = viewportRay(nb, viewportSize);
                Statistics dummy2 = Statistics(0u, 0u);
                Hit nbHit = rayObjectIntersection(nbRay, loadObject(uint(ubo.selectedObjectId)), true, INFINITY, dummy2);
                neighborMask += foundIntersection(nbHit) ? 1.0 : 0.0;
            }
            count++;
        }
    }
    if (count > 0) neighborMask /= float(count);

    float edgeAmount = centerMask != 0u ? 1.0 - neighborMask : neighborMask;
    color = lerp(color, edgeColor, smoothstep(0.0, feather, edgeAmount));

    if (ubo.showFocusPlane != 0) {
        float t;
        PixelInfo pix = samplePixelInfo(coord, viewportSize, renderSize);
        if (pix.aov.hitValid != 0u) {
            float signedDist = dot(pix.aov.positionW, ubo.focusPlane.xyz) + ubo.focusPlane.w;
            t = signedDist > 0.0 ? 1.0 : 0.0;
        } else {
            Ray skyRay = viewportRay(coord, viewportSize);
            float denom = dot(skyRay.dir, ubo.focusPlane.xyz);
            float t_hit = denom != 0.0
                ? -(dot(skyRay.origin, ubo.focusPlane.xyz) + ubo.focusPlane.w) / denom
                : -1.0;
            t = t_hit > 0.0 ? 1.0 : 0.0;
        }
        color *= lerp(float3(1.0), focusColor, t);
    }

    if (ubo.previewBorderEnabled != 0) {
        float2 pixelCoord = float2(coord) + 0.5;
        float2 frameHalfPx = float2(viewportSize) * 0.5 * ubo.previewFrameExtent;
        float2 centeredPx = abs(pixelCoord - float2(viewportSize) * 0.5);
        float2 outsideDist2 = centeredPx - frameHalfPx;
        float outsideDist = max(outsideDist2.x, outsideDist2.y);

        if (outsideDist > 0.0) {
            color *= 0.2;
            if (frac((pixelCoord.x + pixelCoord.y) / previewStripeSpacing) < 0.5) color *= 0.6;
        }

        float edgeMask = 1.0 - smoothstep(outlineWidth - feather, outlineWidth + feather, abs(outsideDist));
        color = lerp(color, edgeColor, edgeMask);
    }

    displayOut[coord] = float4(color, 1.0);
}
