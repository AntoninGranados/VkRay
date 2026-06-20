#ifndef DIELECTRIC_GLSL
#define DIELECTRIC_GLSL

#include "../utils.glsl"
#include "../random.glsl"

#include "material_utils.glsl"

BSDFEval evalDielectricBSDF(in Material mat, in Hit hit, in vec3 wo, in vec3 wi) {
    return BSDFEval(
        vec3(0.0),
        -1.0
    );
}

BSDFSample sampleDielectricBSDF(in Material mat, in Hit hit, in vec3 wo, inout uint seed) {
    float etaI = 1.0;   // TODO: keep track of the current IOR as we traverse the scene
    float etaT = mat.ior;
    if (!hit.frontFace) { float t = etaI; etaI = etaT; etaT = t; }

    vec3 normal = hit.normal + randomInSphere(seed) * mat.roughness;
    if (length(normal) < EPS) normal = hit.normal;
    else normal = normalize(normal);

    float eta = etaI / etaT;

    float cosTheta = clamp(dot(wo, normal), 0.0, 1.0);
    float F = fresnelDielectric(cosTheta, etaI, etaT);

    vec3 wi;
    vec3 weight;
    float pdf;
    if (rand(seed) < F) {
        wi = reflect(-wo, normal);

        pdf = max(F, EPS);
        weight = vec3(1.0);
    } else {
        wi = refract(-wo, normal, eta);
        float etaFactor = (etaI * etaI) / (etaT * etaT);

        pdf = max(1.0 - F, EPS);
        weight = vec3(etaFactor);
    }

    BSDFSample bsdf;
    bsdf.wi      = wi;
    bsdf.weight  = weight;
    bsdf.pdf     = pdf;
    bsdf.isDelta = true;
    bsdf.medium.isInside = (dot(wi, hit.normal) < 0.0) == hit.frontFace;
    bsdf.medium.absorption = mat.albedo;
    bsdf.medium.density = mat.density;
    return bsdf;
}

#endif // DIELECTRIC_GLSL
