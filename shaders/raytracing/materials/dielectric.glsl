#ifndef DIELECTRIC_GLSL
#define DIELECTRIC_GLSL

#include "../utils.glsl"
#include "../random.glsl"

#include "material_utils.glsl"

float dielectricPDF(in Material mat, in Hit hit, in vec3 wo, in vec3 wi) {
    vec3 normal = hit.frontFace ? hit.normal : -hit.normal;
    return abs(dot(normal, wi));
}

vec3 dielectricF(in Material mat, in Hit hit, in vec3 wo, in vec3 wi) {
    return mat.albedo;
}

void sampleDielectricBSDF(in Material mat, in Hit hit, in vec3 wo, out SampleResult result, inout uint seed) {
    float etaI = 1.0;   // TODO: keep track of the current IOR as we traverse the scene
    float etaT = dielectricIoR(mat);
    if (!hit.frontFace) { float t = etaI; etaI = etaT; etaT = t; }

    float eta = etaI / etaT;
    float ri  = etaT / etaI;

    float cosTheta = clamp(dot(wo, hit.normal), 0.0, 1.0);
    float sin2Theta = max(0.0, 1.0 - cosTheta*cosTheta);

    bool tir = (eta * eta) * sin2Theta > 1.0;

    float F = schlickIoR(cosTheta, ri).x;

    vec3 wi;
    if (tir || rand(seed) < F) {
        wi = reflect(-wo, hit.normal);
        float absCos = max(abs(dot(hit.normal, wi)), EPS);

        result.pdf = max(F, EPS);
        result.f   = (mat.albedo * result.pdf) / absCos;
        result.isTransmission = false;
    } else {
        wi = refract(-wo, hit.normal, eta);
        float absCos = max(abs(dot(hit.normal, wi)), EPS);

        float etaFactor = (etaI * etaI) / (etaT * etaT);

        result.pdf = max(1.0 - F, EPS);
        result.f   = (mat.albedo * result.pdf * etaFactor) / absCos;
        result.isTransmission = true;
    }

    result.wi = wi;
    result.isDelta = true;
}

#endif // DIELECTRIC_GLSL
