import "../common.slang";
import "../generated/material_types.slang";

struct DebugUBO {
    int debugView;
};

ConstantBuffer<DebugUBO> ubo;
RWStructuredBuffer<PixelInfo> pixelInfoBuffer;
[[vk::image_format("rgba32f")]] RWTexture2D<float4> debugOut;

float3 srgbToLinear(float3 c) {
    return lerp(c / 12.92, pow((c + 0.055) / 1.055, float3(2.4)), step(0.04045, c));
}

float3 viridis(float t) {
    t = clamp(t, 0.0, 1.0);

    const float3 c0 = float3(0.2777273272234177, 0.005407344544966578, 0.3340998053353061);
    const float3 c1 = float3(0.1050930431085774, 1.404613529898575,   1.384590162594685);
    const float3 c2 = float3(-0.3308618287255563, 0.214847559468213,  0.09509516302823659);
    const float3 c3 = float3(-4.634230498983486, -5.799100973351585, -19.33244095627987);
    const float3 c4 = float3(6.228269936347081,  14.17993336680509,   56.69055260068105);
    const float3 c5 = float3(4.776384997670288, -13.74514537774601,  -65.35303263337234);
    const float3 c6 = float3(-5.435455855934631, 4.645852612178535,   26.3124352495832);

    float3 srgb = c0 + t * (c1 + t * (c2 + t * (c3 + t * (c4 + t * (c5 + t * c6)))));
    return srgbToLinear(srgb);
}

float3 visualize(float3 v)  { return normalize(v) * 0.5 + 0.5; }
float3 visualize(float2 v)  { float z = sqrt(max(0.0, 1.0 - dot(v, v))); return float3(v, z) * 0.5 + 0.5; }
float3 visualize(float v) { return viridis(v); }
float3 visualize(uint v)  { return float3(v != 0u ? 1.0 : 0.0); }

float3 visualizeMatType(uint t) {
    if (t == mat_Principled)   return float3(0.8, 0.8, 0.8);
    if (t == mat_Emissive)     return float3(1.0, 1.0, 0.2);
    if (t == mat_Diffuse)      return float3(0.9, 0.5, 0.2);
    if (t == mat_Metal)        return float3(0.9, 0.8, 0.1);
    if (t == mat_Glossy)       return float3(0.2, 0.5, 0.9);
    if (t == mat_Dielectric)   return float3(0.2, 0.9, 0.8);
    if (t == mat_Volume)       return float3(0.7, 0.2, 0.9);
    if (t == mat_Programmable) return float3(0.9, 0.2, 0.7);
    return float3(0.0);
}

[shader("compute")]
[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID) {
    int2 coord   = int2(dispatchThreadId.xy);
    uint width, height;
    debugOut.GetDimensions(width, height);
    int2 texSize = int2(width, height);
    if (coord.x >= texSize.x || coord.y >= texSize.y) return;

    uint pixelIndex = uint(coord.y * texSize.x + coord.x);
    PixelInfo pix = pixelInfoBuffer[pixelIndex];

    float3 color = float3(0.0);

#define HIT(expr) (pix.aov.hitValid != 0u ? (expr) : float3(0.0))

    if      (ubo.debugView == debug_PositionW)     { color = HIT(visualize(pix.aov.positionW)); }
    else if (ubo.debugView == debug_Position)      { float3 p = pix.aov.position; float3 vis; vis.xy = p.xy / (1.0 + abs(p.xy)) * 0.5 + 0.5; vis.z = p.z / (1.0 + p.z); color = HIT(vis); }
    else if (ubo.debugView == debug_NormalW)       { color = HIT(visualize(pix.aov.normalW)); }
    else if (ubo.debugView == debug_Normal)        { color = HIT(visualize(pix.aov.normal)); }
    else if (ubo.debugView == debug_Albedo)        { color = HIT(clamp(pix.aov.albedo, 0.0, 1.0)); }
    else if (ubo.debugView == debug_Roughness)     { color = HIT(visualize(pix.aov.roughness)); }
    else if (ubo.debugView == debug_MatType)       { color = HIT(visualizeMatType(pix.aov.matType)); }
    else if (ubo.debugView == debug_Bounces)       { color = visualize(float(pix.bounces) / 8.0); }
    else if (ubo.debugView == debug_HitChecks)     { float bvh = float(pix.bvhChecks) / 256.0; float tri = float(pix.triangleChecks) / 256.0; color = max(bvh, tri) > 1.0 ? float3(1.0) : float3(tri, 0.0, bvh); }
    else if (ubo.debugView == debug_Variance)      { color = visualize(pix.varianceProba); }
    else if (ubo.debugView == debug_SkyMask)       { color = visualize(pix.aov.skyMask); }

    debugOut[coord] = float4(color, 1.0);
}
