#ifndef MATERIALS_GLSL
#define MATERIALS_GLSL

#include "../utils.glsl"
#include "../random.glsl"

#include "material_utils.glsl"
#include "principled.glsl"
#include "diffuse.glsl"
#include "metal.glsl"
#include "glossy.glsl"
#include "dielectric.glsl"
#include "volume.glsl"
#include "programmable.glsl"

Material resolveMaterial(in Material mat, inout Hit hit) {
    if (mat.type != mat_Programmable) return mat;
    return createProgrammableMaterial(mat, hit);
}

BSDFEval evalBSDF(in Material mat, inout Hit hit, in vec3 wo, in vec3 wi) {
    mat = resolveMaterial(mat, hit);
    switch (mat.type) {
        case mat_Principled: return evalPrincipledBSDF(mat, hit, wo, wi);
        case mat_Diffuse: return evalDiffuseBSDF(mat, hit, wo, wi);
        case mat_Metal:   return evalMetalBSDF(mat, hit, wo, wi);
        case mat_Glossy:  return evalGlossyBSDF(mat, hit, wo, wi);
        case mat_Dielectric: return evalDielectricBSDF(mat, hit, wo, wi);
        case mat_Volume:     return evalVolumeBSDF(mat, hit, wo, wi);
        default:             return evalDiffuseBSDF(DEFAULT_MATERIAL, hit, wo, wi);
    }
}

BSDFSample sampleBSDF(in Material mat, inout Hit hit, in vec3 wo, inout uint seed) {
    mat = resolveMaterial(mat, hit);
    switch (mat.type) {
        case mat_Principled: return samplePrincipledBSDF(mat, hit, wo, seed);  break;
        case mat_Diffuse: return sampleDiffuseBSDF(mat, hit, wo, seed);  break;
        case mat_Metal:   return sampleMetalBSDF(mat, hit, wo, seed);    break;
        case mat_Glossy:  return sampleGlossyBSDF(mat, hit, wo, seed);   break;
        case mat_Dielectric: return sampleDielectricBSDF(mat, hit, wo, seed);  break;
        case mat_Volume:     return sampleVolumeBSDF(mat, hit, wo, seed);      break;
        default: return sampleDiffuseBSDF(DEFAULT_MATERIAL, hit, wo, seed); break;
    }
}

#endif
