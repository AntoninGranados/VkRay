#ifndef MATERIALS_GLSL
#define MATERIALS_GLSL

#include "../utils.glsl"
#include "../random/utils.glsl"

#include "material_utils.glsl"
#include "principled.glsl"
#include "diffuse.glsl"
#include "metal.glsl"
#include "glossy.glsl"
#include "dielectric.glsl"
#include "volume.glsl"
#include "generated/programmable_dispatch.glsl"

Material resolveMaterial(in Material mat, inout Hit hit, in vec3 wo, inout RngState rng) {
    if (mat.type != mat_Programmable) return mat;
    return dispatchProgrammable(mat, hit, wo, rng);
}

BSDFEval evalBSDF(in Material mat, inout Hit hit, in vec3 wo, in vec3 wi, inout RngState rng) {
    mat = resolveMaterial(mat, hit, wo, rng);
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

BSDFSample sampleBSDF(in Material mat, inout Hit hit, in vec3 wo, inout RngState rng) {
    mat = resolveMaterial(mat, hit, wo, rng);
    switch (mat.type) {
        case mat_Principled: return samplePrincipledBSDF(mat, hit, wo, rng); break;
        case mat_Diffuse:    return sampleDiffuseBSDF(mat, hit, wo, rng);    break;
        case mat_Metal:      return sampleMetalBSDF(mat, hit, wo, rng);      break;
        case mat_Glossy:     return sampleGlossyBSDF(mat, hit, wo, rng);     break;
        case mat_Dielectric: return sampleDielectricBSDF(mat, hit, wo, rng); break;
        case mat_Volume:     return sampleVolumeBSDF(mat, hit, wo, rng);     break;
        default: return sampleDiffuseBSDF(DEFAULT_MATERIAL, hit, wo, rng); break;
    }
}

#endif
