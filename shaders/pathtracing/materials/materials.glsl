#ifndef MATERIALS_GLSL
#define MATERIALS_GLSL

#include "../utils.glsl"
#include "../random.glsl"

#include "material_utils.glsl"
#include "lambertian.glsl"
#include "ggx_metal.glsl"
#include "ggx_glossy.glsl"
#include "dielectric.glsl"
#include "programmable.glsl"

Material resolveMaterial(in Material mat, in Hit hit) {
    if (mat.type != mat_Programmable) return mat;
    return createProgrammableMaterial(mat, hit);
}

float samplePDF(in Material mat, in Hit hit, in vec3 wo, in vec3 wi) {
    mat = resolveMaterial(mat, hit);
    switch (mat.type) {
        case mat_Lambertian: return lambertianPDF(mat, hit, wo, wi);
        case mat_GgxMetal:   return ggxMetalPDF(mat, hit, wo, wi);
        case mat_GgxGlossy:  return ggxGlossyPDF(mat, hit, wo, wi);
        case mat_Dielectric: return dielectricPDF(mat, hit, wo, wi);
        default:             return lambertianPDF(DEFAULT_MATERIAL, hit, wo, wi);
    }
}

vec3 sampleF(in Material mat, in Hit hit, in vec3 wo, in vec3 wi) {
    mat = resolveMaterial(mat, hit);
    switch (mat.type) {
        case mat_Lambertian: return lambertianF(mat, hit, wo, wi);
        case mat_GgxMetal:   return ggxMetalF(mat, hit, wo, wi);
        case mat_GgxGlossy:  return ggxGlossyF(mat, hit, wo, wi);
        case mat_Dielectric: return dielectricF(mat, hit, wo, wi);
        default:             return lambertianF(DEFAULT_MATERIAL, hit, wo, wi);
    }
}

void sampleBSDF(in Material mat, in Hit hit, in vec3 wo, out SampleResult result, inout uint seed) {
    mat = resolveMaterial(mat, hit);
    switch (mat.type) {
        case mat_Lambertian: sampleLambertianBSDF(mat, hit, wo, result, seed);              break;
        case mat_GgxMetal:   sampleGgxMetalBSDF(mat, hit, wo, result, seed);                break;
        case mat_GgxGlossy:  sampleGgxGlossyBSDF(mat, hit, wo, result, seed);               break;
        case mat_Dielectric: sampleDielectricBSDF(mat, hit, wo, result, seed);              break;
        default:             sampleLambertianBSDF(DEFAULT_MATERIAL, hit, wo, result, seed); break;
    }
}

#endif
