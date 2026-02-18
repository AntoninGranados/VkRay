#ifndef DIELECTRIC_GLSL
#define DIELECTRIC_GLSL

#include "../utils.glsl"
#include "../random.glsl"

#include "material_utils.glsl"

// Should never be called as the material is delta
vec3 dielectricF(in Material mat, in Hit hit, in vec3 wo, in vec3 wi) {
    return vec3(0.0);
}

// Should never be called as the material is delta
float dielectricPDF(in Material mat, in Hit hit, in vec3 wo, in vec3 wi) {
    return -1.0f;
}

void sampleDielectricBSDF(in Material mat, in Hit hit, in vec3 wo, out SampleResult result, inout uint seed) {
    float etaI = 1.0;   // TODO: keep track of the current IOR as we traverse the scene
    float etaT = dielectricIoR(mat);
    if (!hit.frontFace) { float t = etaI; etaI = etaT; etaT = t; }

    vec3 normal = hit.normal + randomInSphere(seed) * dielectricRoughness(mat);
    if (length(normal) < EPS) normal = hit.normal;
    else normal = normalize(normal);

    float eta = etaI / etaT;
    float ri  = etaT / etaI;

    float cosTheta = clamp(dot(wo, normal), 0.0, 1.0);
    float sin2Theta = max(0.0, 1.0 - cosTheta*cosTheta);

    bool tir = (eta * eta) * sin2Theta > 1.0;

    float F = schlickIoR(cosTheta, ri).x;

    vec3 wi;
    if (tir || rand(seed) < F) {
        wi = reflect(-wo, normal);

        float absCos = max(abs(dot(normal, wi)), EPS);

        result.pdf = max(F, EPS);
        result.f   = (mat.albedo * result.pdf) / absCos;
    } else {
        wi = refract(-wo, normal, eta);

        float absCos = max(abs(dot(normal, wi)), EPS);
        float etaFactor = (etaI * etaI) / (etaT * etaT);

        result.pdf = max(1.0 - F, EPS);
        result.f = (mat.albedo * result.pdf * etaFactor) / absCos;
    }

    result.wi = wi;
    result.isDelta = true;
}

#endif // DIELECTRIC_GLSL
