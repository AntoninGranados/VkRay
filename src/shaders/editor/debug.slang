import "../core/utils.slang";
import "../core/colors/cmap.slang";

struct DebugUBO {
    int debugView;
};

ConstantBuffer<DebugUBO> ubo;
RWStructuredBuffer<PixelInfo> pixelInfoBuffer;
[[vk::image_format("rgba32f")]] RWTexture2D<float4> debugOut;

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
    else if (ubo.debugView == debug_Uv)            { color = HIT(float3(frac(pix.aov.uv), 0.0)); }

    debugOut[coord] = float4(color, 1.0);
}
