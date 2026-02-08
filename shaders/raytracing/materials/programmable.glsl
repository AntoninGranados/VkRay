#ifndef PROGRAMMABLE_GLSL
#define PROGRAMMABLE_GLSL

#include "../utils.glsl"
#include "../random.glsl"

#include "material_utils.glsl"
#include "lambertian.glsl"
#include "ggx_metal.glsl"
#include "ggx_plastic.glsl"

#define SCALE 0.5

#define PROGRAMMABLE_LAMBERT(mat) LAMBERTIAN_MATERIAL(mat.albedo)
#define PROGRAMMABLE_PLASTIC(mat) GGX_PLASTIC_MATERIAL(mat.albedo, 0.1, 0.2)

float programmablePDF(in Material mat, in Hit hit, in vec3 wo, in vec3 wi) {
    vec2 p = hit.p.xz;
    vec2 ip = round(p / SCALE);

    if (int(ip.x + ip.y + 1) % 2 == 0) {
        return lambertianPDF(PROGRAMMABLE_LAMBERT(mat), hit, wo, wi);
    } else {
        return ggxPlasticPDF(PROGRAMMABLE_PLASTIC(mat), hit, wo, wi);
    }
}

vec3 programmableF(in Material mat, in Hit hit, in vec3 wo, in vec3 wi) {
    vec2 p = hit.p.xz;
    vec2 ip = round(p / SCALE);

    if (int(ip.x + ip.y + 1) % 2 == 0) {
        return lambertianF(PROGRAMMABLE_LAMBERT(mat), hit, wo, wi);
    } else {
        return ggxPlasticF(PROGRAMMABLE_PLASTIC(mat), hit, wo, wi);
    }
}

void sampleProgrammableBSDF(in Material mat, in Hit hit, in vec3 wo, out SampleResult result, inout uint seed) {
    vec2 p = hit.p.xz;
    vec2 ip = round(p / SCALE);

    if (int(ip.x + ip.y + 1) % 2 == 0) {
        sampleLambertianBSDF(PROGRAMMABLE_LAMBERT(mat), hit, wo, result, seed);
    } else {
        sampleGgxPlasticBSDF(PROGRAMMABLE_PLASTIC(mat), hit, wo, result, seed);
    }
}

#endif // PROGRAMMABLE_GLSL