import "../scene.slang";
import "../random/utils.slang";
import "../colors/utils.slang";

__exported import "principled.slang";
__exported import "diffuse.slang";
__exported import "metal.slang";
__exported import "glossy.slang";
__exported import "dielectric.slang";
__exported import "volume.slang";
import "../../generated/material_dispatch.slang";

ResolvedMaterial resolveMaterial(in Material mat, inout Hit hit, in float3 wo, inout RngState rng) {
    if (mat.type == mat_Programmable) return dispatchProgrammable(mat, hit, wo, rng);
    return unpackMaterial(mat);
}

BSDFEval evalBSDF(in ResolvedMaterial mat, inout Hit hit, in float3 wo, in float3 wi, inout RngState rng) {
    switch (mat.type) {
        case mat_Principled: return evalPrincipledBSDF(mat, hit, wo, wi);
        case mat_Diffuse: return evalDiffuseBSDF(mat, hit, wo, wi);
        case mat_Metal:   return evalMetalBSDF(mat, hit, wo, wi);
        case mat_Glossy:  return evalGlossyBSDF(mat, hit, wo, wi);
        case mat_Dielectric: return evalDielectricBSDF(mat, hit, wo, wi);
        case mat_Volume:     return evalVolumeBSDF(mat, hit, wo, wi);
        default:             return evalDiffuseBSDF(defaultMaterial(), hit, wo, wi);
    }
}

BSDFSample sampleBSDF(in ResolvedMaterial mat, inout Hit hit, in float3 wo, inout RngState rng) {
    switch (mat.type) {
        case mat_Principled: return samplePrincipledBSDF(mat, hit, wo, rng);
        case mat_Diffuse:    return sampleDiffuseBSDF(mat, hit, wo, rng);
        case mat_Metal:      return sampleMetalBSDF(mat, hit, wo, rng);
        case mat_Glossy:     return sampleGlossyBSDF(mat, hit, wo, rng);
        case mat_Dielectric: return sampleDielectricBSDF(mat, hit, wo, rng);
        case mat_Volume:     return sampleVolumeBSDF(mat, hit, wo, rng);
        default: return sampleDiffuseBSDF(defaultMaterial(), hit, wo, rng);
    }
}
