#ifndef PROGRAMMABLE_GLSL
#define PROGRAMMABLE_GLSL

#include "../utils.glsl"
#include "../random.glsl"

#include "material_utils.glsl"
#include "lambertian.glsl"
#include "ggx_metal.glsl"
#include "ggx_glossy.glsl"

#define SCALE 0.5

#define CHECKBOARD_CHOICE(p) int(round(p.x / SCALE) + round(p.y / SCALE) + 1) % 2 == 0

#define PROGRAMMABLE_LAMBERT(mat) LAMBERTIAN_MATERIAL(mat.albedo)
#define PROGRAMMABLE_GLOSSY(mat) GGX_GLOSSY_MATERIAL(mat.albedo, 0.0, 0.5)

float programmablePDF(in Material mat, in Hit hit, in vec3 wo, in vec3 wi) {
    if (CHECKBOARD_CHOICE(hit.p.xz)) {
        return lambertianPDF(PROGRAMMABLE_LAMBERT(mat), hit, wo, wi);
    } else {
        return ggxGlossyPDF(PROGRAMMABLE_GLOSSY(mat), hit, wo, wi);
    }
}

vec3 programmableF(in Material mat, in Hit hit, in vec3 wo, in vec3 wi) {
    if (CHECKBOARD_CHOICE(hit.p.xz)) {
        return lambertianF(PROGRAMMABLE_LAMBERT(mat), hit, wo, wi);
    } else {
        return ggxGlossyF(PROGRAMMABLE_GLOSSY(mat), hit, wo, wi);
    }
}

void sampleProgrammableBSDF(in Material mat, in Hit hit, in vec3 wo, out SampleResult result, inout uint seed) {
    if (CHECKBOARD_CHOICE(hit.p.xz)) {
        sampleLambertianBSDF(PROGRAMMABLE_LAMBERT(mat), hit, wo, result, seed);
    } else {
        sampleGgxGlossyBSDF(PROGRAMMABLE_GLOSSY(mat), hit, wo, result, seed);
    }
}

#endif // PROGRAMMABLE_GLSL
