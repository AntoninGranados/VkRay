#ifndef MATERIALS_GLSL
#define MATERIALS_GLSL

#include "../utils.glsl"
#include "../random.glsl"

#include "material_utils.glsl"
#include "principled.glsl"
#include "lambertian.glsl"
#include "ggx_metal.glsl"
#include "ggx_glossy.glsl"
#include "dielectric.glsl"
#include "volume.glsl"
#include "programmable.glsl"

Material resolveMaterial(in Material mat, in Hit hit) {
    if (mat.type != mat_Programmable) return mat;
    return createProgrammableMaterial(mat, hit);
}

BSDFEval evalBSDF(in Material mat, in Hit hit, in vec3 wo, in vec3 wi) {
    mat = resolveMaterial(mat, hit);
    switch (mat.type) {
        case mat_Principled: return evalPrincipledBSDF(mat, hit, wo, wi);
        case mat_Lambertian: return evalLambertianBSDF(mat, hit, wo, wi);
        case mat_GgxMetal:   return evalGgxMetalBSDF(mat, hit, wo, wi);
        case mat_GgxGlossy:  return evalGgxGlossyBSDF(mat, hit, wo, wi);
        case mat_Dielectric: return evalDielectricBSDF(mat, hit, wo, wi);
        case mat_Volume:     return evalVolumeBSDF(mat, hit, wo, wi);
        default:             return evalLambertianBSDF(DEFAULT_MATERIAL, hit, wo, wi);
    }
}

BSDFSample sampleBSDF(in Material mat, in Hit hit, in vec3 wo, inout uint seed) {
    mat = resolveMaterial(mat, hit);
    switch (mat.type) {
        case mat_Principled: return samplePrincipledBSDF(mat, hit, wo, seed);  break;
        case mat_Lambertian: return sampleLambertianBSDF(mat, hit, wo, seed);  break;
        case mat_GgxMetal:   return sampleGgxMetalBSDF(mat, hit, wo, seed);    break;
        case mat_GgxGlossy:  return sampleGgxGlossyBSDF(mat, hit, wo, seed);   break;
        case mat_Dielectric: return sampleDielectricBSDF(mat, hit, wo, seed);  break;
        case mat_Volume:     return sampleVolumeBSDF(mat, hit, wo, seed);      break;
        default: return sampleLambertianBSDF(DEFAULT_MATERIAL, hit, wo, seed); break;
    }
}

#endif
