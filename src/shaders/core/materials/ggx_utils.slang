import "../utils.slang";
import "../random/utils.slang";

import "material_utils.slang";

float3 ggxScatter(in SurfacePoint surface, in float3 wo, in float alpha, out float3 h, inout RngState rng) {
    float s1 = rand(rng);
    float s2 = rand(rng);

    // Build tangent basis
    float3 t;
    if (abs(surface.normal.z) < 1 - EPS) {
        t = normalize(cross(float3(0.0, 0.0, 1.0), surface.normal));
    } else {
        t = normalize(cross(float3(0.0, 1.0, 0.0), surface.normal));
    }
    float3 b = cross(surface.normal, t);

    float3 view = normalize(float3(alpha * dot(wo, t), alpha * dot(wo, b), dot(wo, surface.normal)));
    float lengthSquared = view.x * view.x + view.y * view.y;
    float3 t1 = lengthSquared > 0.0 ? float3(-view.y, view.x, 0.0) * rsqrt(lengthSquared) : float3(1.0, 0.0, 0.0);
    float3 t2 = cross(view, t1);

    float r = sqrt(s1);
    float phi = 2.0 * PI * s2;
    float p1 = r * cos(phi);
    float p2 = r * sin(phi);
    float s = 0.5 * (1.0 + view.z);
    p2 = (1.0 - s) * sqrt(1.0 - p1 * p1) + s * p2;

    float3 visible = p1 * t1 + p2 * t2 + sqrt(max(0.0, 1.0 - p1 * p1 - p2 * p2)) * view;
    float3 local = normalize(float3(alpha * visible.x, alpha * visible.y, max(0.0, visible.z)));

    h = normalize(local.x * t + local.y * b + local.z * surface.normal);
    return reflect(-wo, h);
}

float ggxDirectionalAlbedo(in float roughness, in float NoV, in float ior) {
    float F0 = pow((ior - 1.0) / (ior + 1.0), 2.0);
    float4 r = roughness * float4(-1.0, -0.0275, -0.572, 0.022) + float4(1.0, 0.0425, 1.04, -0.04);
    float a004 = min(r.x * r.x, exp2(-9.28 * NoV)) * r.x + r.y;
    float2 ab = float2(-1.04, 1.04) * a004 + r.zw;
    return saturate(F0 * ab.x + saturate(50.0 * F0) * ab.y);
}

float D_ggx(in float alpha, in float3 n, in float3 h) {
    float NoH = max(dot(n, h), 0.0);
    if (NoH <= 0.0) return 0.0;

    float a_sq = alpha * alpha;
    float denom_part = NoH*NoH * (a_sq - 1.0) + 1.0;
    return a_sq / (PI * denom_part * denom_part);
}

float G1_ggx(in float alpha, in float3 n, in float3 v) {
    float NoV = dot(n, v);
    if (NoV <= 0.0) return 0.0;

    float a_sq = alpha * alpha;
    float denom = NoV + sqrt(a_sq + (1.0 - a_sq) * NoV * NoV);
    return 2.0 * NoV / denom;
}

struct GgxTerms {
    float D;
    float G;
    float G1Wo;
    float NoH;
    float VoH;
    float cosWo;
    float cosWi;
};

GgxTerms computeGgxTerms(float alpha, in SurfacePoint surface, in float3 wo, in float3 wi, in float3 h) {
    GgxTerms t;

    float NoH   = dot(surface.normal, h);
    float VoH   = dot(wo, h);
    float NoV   = dot(surface.normal, wo);
    float NoL   = dot(surface.normal, wi);

    if (NoH <= EPS || VoH <= EPS || NoV <= EPS || NoL <= EPS) {
        t.D = 0.0;
        t.G = 0.0;
        t.G1Wo = 0.0;
        t.NoH = 0.0;
        t.VoH = 0.0;
        t.cosWo = 0.0;
        t.cosWi = 0.0;
        return t;
    }

    t.NoH   = NoH;
    t.VoH   = VoH;
    t.cosWo = NoV;
    t.cosWi = NoL;
    t.D     = D_ggx(alpha, surface.normal, h);
    t.G1Wo  = G1_ggx(alpha, surface.normal, wo);
    t.G     = t.G1Wo * G1_ggx(alpha, surface.normal, wi);
    return t;
}

float3 ggxBRDF(in GgxTerms t) {
    if (t.D <= 0.0 || t.cosWo <= 0.0 || t.cosWi <= 0.0) return float3(0.0);
    return float3(t.D * t.G / (4.0 * t.cosWo * t.cosWi + EPS));
}

float ggxPDF(in GgxTerms t) {
    if (t.D <= 0.0 || t.cosWo <= 0.0) return 0.0;
    return (t.D * t.G1Wo) / (4.0 * t.cosWo);
}
