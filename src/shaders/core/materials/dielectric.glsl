#ifndef DIELECTRIC_GLSL
#define DIELECTRIC_GLSL

#include "../utils.glsl"
#include "../random/utils.glsl"

#include "material_utils.glsl"

BSDFEval evalDielectricBSDF(in ResolvedMaterial mat, in Hit hit, in vec3 wo, in vec3 wi) {
    return BSDFEval(
        vec3(0.0),
        -1.0
    );
}

BSDFSample sampleDielectricBSDF(in ResolvedMaterial mat, in Hit hit, in vec3 wo, inout RngState rng) {
    float etaI = 1.0;   // TODO: keep track of the current IOR as we traverse the scene
    float etaT = dielectricIor(mat);
    if (!hit.frontFace) { float t = etaI; etaI = etaT; etaT = t; }

    vec3 normal = hit.normal + randomInSphere(rng) * dielectricRoughness(mat);
    if (length(normal) < EPS) normal = hit.normal;
    else normal = normalize(normal);

    float eta = etaI / etaT;

    float cosTheta = clamp(dot(wo, normal), 0.0, 1.0);
    float F = fresnelDielectric(cosTheta, etaI, etaT);

    vec3 wi;
    vec3 weight;
    float pdf;
    if (rand(rng) < F) {
        wi = reflect(-wo, normal);

        pdf = max(F, EPS);
        weight = vec3(1.0);
    } else {
        wi = refract(-wo, normal, eta);
        float etaFactor = (etaI * etaI) / (etaT * etaT);

        pdf = max(1.0 - F, EPS);
        weight = vec3(etaFactor);
    }

    bool entering = (dot(wi, hit.normal) < 0.0) == hit.frontFace;

    BSDFSample bsdf;
    bsdf.wi                   = wi;
    bsdf.weight               = weight;
    bsdf.pdf                  = pdf;
    bsdf.isDelta              = true;
    bsdf.medium.isDielectric  = entering;
    bsdf.medium.isVolume      = entering && dielectricDensity(mat) > 0.0;
    bsdf.medium.absorption    = albedo(mat);
    bsdf.medium.density       = dielectricDensity(mat);
    bsdf.medium.scatterAlbedo = dielectricTransmission(mat);
    bsdf.medium.anisotropic   = dielectricAnisotropic(mat);
    return bsdf;
}

#endif // DIELECTRIC_GLSL
