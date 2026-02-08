#ifndef GGX_PLASTIC_GLSL
#define GGX_PLASTIC_GLSL

#include "../utils.glsl"
#include "../random.glsl"

#include "material_utils.glsl"
#include "lambertian.glsl"
#include "ggx_utils.glsl"

vec3 ggxPlasticF(in Material mat, in Hit hit, in vec3 wo, in vec3 wi) {
    float alpha = ggxMetalRoughness(mat) * ggxMetalRoughness(mat);
    vec3 m = normalize(wo + wi);
    
    float NoV = max(dot(hit.normal, wo), 0.0);
    vec3 F  = schlickIoR(NoV, ggxPlasticIoR(mat));

    vec3 fs = F * partialGgxF(mat, hit, wo, wi, alpha);         // specular
    vec3 fd = (vec3(1.0) - F) * lambertianF(mat, hit, wo, wi);  // diffuse

    return fs + fd;
}

float ggxPlasticPDF(in Material mat, in Hit hit, in vec3 wo, in vec3 wi) {
    float NoV = max(dot(hit.normal, wo), 0.0);
    vec3  Fv  = schlickIoR(NoV, ggxPlasticIoR(mat));
    float ps  = clamp(luma(Fv), 0.05, 0.95);
    float pd  = 1.0 - ps;

    float pdfS = ggxPDF(mat, hit, wo, wi);
    float pdfD = lambertianPDF(mat, hit, wo, wi);

    return ps * pdfS + pd * pdfD;
}

void sampleGgxPlasticBSDF(in Material mat, in Hit hit, in vec3 wo, out SampleResult result, inout uint seed) {
    float rough = ggxPlasticRoughness(mat);
    bool specIsDelta = (rough < 0.05);

    float NoV = max(dot(hit.normal, wo), 0.0);
    vec3  Fv  = schlickIoR(NoV, ggxPlasticIoR(mat));
    float pSpec = clamp(luma(Fv), 0.05, 0.95);
    float pDiff = 1.0 - pSpec;

    float xi = rand(seed);

    if (xi < pSpec) {
        if (specIsDelta) {
            sampleMirrorBSDF(mat, hit, wo, result);
            return;
        } else {
            float alpha = ggxPlasticRoughness(mat) * ggxPlasticRoughness(mat);
            result.wi = ggxScatter(mat, hit, wo, alpha, seed);
            result.isDelta = false;
        }
    } else {
        result.wi = cosineScatter(mat, hit.normal, wo, seed);
        result.isDelta = false;
    }

    result.f   = ggxPlasticF(mat, hit, wo, result.wi);
    result.pdf = ggxPlasticPDF(mat, hit, wo, result.wi);
}

#endif // GGX_PLASTIC_GLSL
