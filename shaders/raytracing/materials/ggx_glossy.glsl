#ifndef GGX_GLOSSY_GLSL
#define GGX_GLOSSY_GLSL

#include "../utils.glsl"
#include "../random.glsl"

#include "material_utils.glsl"
#include "lambertian.glsl"
#include "ggx_utils.glsl"

vec3 ggxGlossyF(in Material mat, in Hit hit, in vec3 wo, in vec3 wi) {
    float alpha = ggxGlossyRoughness(mat) * ggxGlossyRoughness(mat);
    vec3 m = normalize(wo + wi);
    
    float NoV = max(dot(hit.normal, wo), 0.0);
    vec3 F  = schlickIoR(NoV, ggxGlossyIoR(mat));

    vec3 fs = F * partialGgxF(mat, hit, wo, wi, alpha);         // specular
    vec3 fd = (vec3(1.0) - F) * lambertianF(mat, hit, wo, wi);  // diffuse

    return fs + fd;
}

float ggxGlossyPDF(in Material mat, in Hit hit, in vec3 wo, in vec3 wi) {
    float NoV = max(dot(hit.normal, wo), 0.0);
    vec3  Fv  = schlickIoR(NoV, ggxGlossyIoR(mat));
    float ps  = clamp(luma(Fv), 0.05, 0.95);
    float pd  = 1.0 - ps;

    float pdfS = ggxPDF(mat, hit, wo, wi);
    float pdfD = lambertianPDF(mat, hit, wo, wi);

    return ps * pdfS + pd * pdfD;
}

void sampleGgxGlossyBSDF(in Material mat, in Hit hit, in vec3 wo, out SampleResult result, inout uint seed) {
    float rough = ggxGlossyRoughness(mat);
    bool specIsDelta = (rough < 0.05);

    float NoV = max(dot(hit.normal, wo), 0.0);
    vec3  Fv  = schlickIoR(NoV, ggxGlossyIoR(mat));
    float pSpec = clamp(luma(Fv), 0.05, 0.95);
    float pDiff = 1.0 - pSpec;

    float xi = rand(seed);

    if (xi < pSpec) {
        if (specIsDelta) {
            sampleMirrorBSDF(mat, hit, wo, result);
            return;
        } else {
            float alpha = ggxGlossyRoughness(mat) * ggxGlossyRoughness(mat);
            result.wi = ggxScatter(mat, hit, wo, alpha, seed);
            result.isDelta = false;
        }
    } else {
        result.wi = cosineScatter(mat, hit.normal, wo, seed);
        result.isDelta = false;
    }

    result.f   = ggxGlossyF(mat, hit, wo, result.wi);
    result.pdf = ggxGlossyPDF(mat, hit, wo, result.wi);
    result.isTransmission = false;
}

#endif // GGX_GLOSSY_GLSL
