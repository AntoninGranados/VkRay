#version 450

#include "../common.glsl"

layout(set = 0, binding = 0) uniform DebugUBO {
    int debugView;
} ubo;
layout(set = 0, binding = 1) buffer PixelInfoBuffer {
    PixelInfo pixels[];
} pixelInfoBuffer;
layout(set = 0, binding = 2, rgba32f) uniform writeonly image2D debugOut;

layout(local_size_x = 8, local_size_y = 8) in;

vec3 visualize(vec3 v)  { return normalize(v) * 0.5 + 0.5; }
vec3 visualize(vec2 v)  { float z = sqrt(max(0.0, 1.0 - dot(v, v))); return vec3(v, z) * 0.5 + 0.5; }
vec3 visualize(float v) { return vec3(v); }
vec3 visualize(uint v)  { return vec3(v != 0u ? 1.0 : 0.0); }

vec3 visualizeMatType(uint t) {
    if (t == mat_Principled)   return vec3(0.8, 0.8, 0.8);
    if (t == mat_Emissive)     return vec3(1.0, 1.0, 0.2);
    if (t == mat_Lambertian)   return vec3(0.9, 0.5, 0.2);
    if (t == mat_GgxMetal)     return vec3(0.9, 0.8, 0.1);
    if (t == mat_GgxGlossy)    return vec3(0.2, 0.5, 0.9);
    if (t == mat_Dielectric)   return vec3(0.2, 0.9, 0.8);
    if (t == mat_Volume)       return vec3(0.7, 0.2, 0.9);
    if (t == mat_Programmable) return vec3(0.9, 0.2, 0.7);
    return vec3(0.0);
}

void main() {
    ivec2 coord   = ivec2(gl_GlobalInvocationID.xy);
    ivec2 texSize = imageSize(debugOut);
    if (coord.x >= texSize.x || coord.y >= texSize.y) return;

    uint pixelIndex = uint(coord.y * texSize.x + coord.x);
    PixelInfo pix = pixelInfoBuffer.pixels[pixelIndex];

    vec3 color = vec3(0.0);

#define HIT(expr) (pix.aov.hitValid != 0u ? (expr) : vec3(0.0))

    if      (ubo.debugView == debug_PositionW)     { color = HIT(visualize(pix.aov.positionW)); }
    else if (ubo.debugView == debug_Position)      { vec3 p = pix.aov.position; vec3 vis; vis.xy = p.xy / (1.0 + abs(p.xy)) * 0.5 + 0.5; vis.z = p.z / (1.0 + p.z); color = HIT(vis); }
    else if (ubo.debugView == debug_NormalW)       { color = HIT(visualize(pix.aov.normalW)); }
    else if (ubo.debugView == debug_Normal)        { color = HIT(visualize(pix.aov.normal)); }
    else if (ubo.debugView == debug_Albedo)        { color = HIT(clamp(pix.aov.albedo, 0.0, 1.0)); }
    else if (ubo.debugView == debug_Roughness)     { color = HIT(visualize(pix.aov.roughness)); }
    else if (ubo.debugView == debug_MatType)       { color = HIT(visualizeMatType(pix.aov.matType)); }
    else if (ubo.debugView == debug_Bounces)       { color = visualize(float(pix.bounces) / 8.0); }
    else if (ubo.debugView == debug_HitChecks)     { float bvh = float(pix.bvhChecks) / 256.0; float tri = float(pix.triangleChecks) / 256.0; color = max(bvh, tri) > 1.0 ? vec3(1.0) : vec3(tri, 0.0, bvh); }
    else if (ubo.debugView == debug_Variance)      { color = vec3(pix.varianceProba); }
    else if (ubo.debugView == debug_SkyMask)       { color = visualize(pix.aov.skyMask); }

    imageStore(debugOut, coord, vec4(color, 1.0));
}
