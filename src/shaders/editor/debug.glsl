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

vec3 srgbToLinear(vec3 c) {
    return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)), step(0.04045, c));
}

vec3 viridis(float t) {
    t = clamp(t, 0.0, 1.0);

    const vec3 c0 = vec3(0.2777273272234177, 0.005407344544966578, 0.3340998053353061);
    const vec3 c1 = vec3(0.1050930431085774, 1.404613529898575,   1.384590162594685);
    const vec3 c2 = vec3(-0.3308618287255563, 0.214847559468213,  0.09509516302823659);
    const vec3 c3 = vec3(-4.634230498983486, -5.799100973351585, -19.33244095627987);
    const vec3 c4 = vec3(6.228269936347081,  14.17993336680509,   56.69055260068105);
    const vec3 c5 = vec3(4.776384997670288, -13.74514537774601,  -65.35303263337234);
    const vec3 c6 = vec3(-5.435455855934631, 4.645852612178535,   26.3124352495832);

    vec3 srgb = c0 + t * (c1 + t * (c2 + t * (c3 + t * (c4 + t * (c5 + t * c6)))));
    return srgbToLinear(srgb);
}

vec3 visualize(vec3 v)  { return normalize(v) * 0.5 + 0.5; }
vec3 visualize(vec2 v)  { float z = sqrt(max(0.0, 1.0 - dot(v, v))); return vec3(v, z) * 0.5 + 0.5; }
vec3 visualize(float v) { return viridis(v); }
vec3 visualize(uint v)  { return vec3(v != 0u ? 1.0 : 0.0); }

vec3 visualizeMatType(uint t) {
    if (t == mat_Principled)   return vec3(0.8, 0.8, 0.8);
    if (t == mat_Emissive)     return vec3(1.0, 1.0, 0.2);
    if (t == mat_Diffuse)      return vec3(0.9, 0.5, 0.2);
    if (t == mat_Metal)        return vec3(0.9, 0.8, 0.1);
    if (t == mat_Glossy)       return vec3(0.2, 0.5, 0.9);
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
    else if (ubo.debugView == debug_Variance)      { color = visualize(pix.varianceProba); }
    else if (ubo.debugView == debug_SkyMask)       { color = visualize(pix.aov.skyMask); }

    imageStore(debugOut, coord, vec4(color, 1.0));
}
