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

#define PROGRAMMABLE_LAMBERT(albedo) LAMBERTIAN_MATERIAL(albedo)
#define PROGRAMMABLE_GLOSSY(albedo) GGX_GLOSSY_MATERIAL(albedo, 0.1, 0.2)

vec2 programmablePlaneCoords(in Hit hit) {
    vec3 up = abs(hit.normal.z) < (1.0 - EPS) ? vec3(0.0, 0.0, 1.0) : vec3(0.0, 1.0, 0.0);
    vec3 t = normalize(cross(up, hit.normal));
    vec3 b = cross(hit.normal, t);
    return vec2(dot(hit.p, t), dot(hit.p, b));
}

float programmablePDF(in Material mat, in Hit hit, in vec3 wo, in vec3 wi) {
    vec2 planeCoords = programmablePlaneCoords(hit);
    if (CHECKBOARD_CHOICE(planeCoords)) {
        return lambertianPDF(PROGRAMMABLE_LAMBERT(mat.albedo), hit, wo, wi);
    } else {
        return ggxGlossyPDF(PROGRAMMABLE_GLOSSY(mat.albedo), hit, wo, wi);
    }
}

vec3 programmableF(in Material mat, in Hit hit, in vec3 wo, in vec3 wi) {
    vec2 planeCoords = programmablePlaneCoords(hit);
    if (CHECKBOARD_CHOICE(planeCoords)) {
        return lambertianF(PROGRAMMABLE_LAMBERT(mat.albedo), hit, wo, wi);
    } else {
        return ggxGlossyF(PROGRAMMABLE_GLOSSY(mat.albedo), hit, wo, wi);
    }
}

void sampleProgrammableBSDF(in Material mat, in Hit hit, in vec3 wo, out SampleResult result, inout uint seed) {
    vec2 planeCoords = programmablePlaneCoords(hit);
    if (CHECKBOARD_CHOICE(planeCoords)) {
        sampleLambertianBSDF(PROGRAMMABLE_LAMBERT(mat.albedo), hit, wo, result, seed);
    } else {
        sampleGgxGlossyBSDF(PROGRAMMABLE_GLOSSY(mat.albedo), hit, wo, result, seed);
    }
}

#endif // PROGRAMMABLE_GLSL
