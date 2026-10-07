#ifndef LIGHT_GLSL
#define LIGHT_GLSL

#include "inputs.glsl"
#include "global.glsl"
#include "random/utils.glsl"

int getRandomLightId(inout RngState rng) {
    if (lightBuffer.lightCount == 0) return -1;

    float r = rand(rng);
    int low = 0;
    int high = int(lightBuffer.lightCount) - 1;
    while (low < high) {
        int middle = (low + high) / 2;
        if (lightBuffer.lights[middle].cumulativeProbability > r) high = middle;
        else low = middle + 1;
    }
    return low;
}

Hit intersection(in Ray ray, bool anyHit, float tMax, inout Statistics stats); // Forward declaration

struct LightSample {
    vec3 wi;
    float pdf;
    vec3 Le;
    bool skip;
};

float lightPDF(in int lightId, in float dist, in vec3 normal, in vec3 wi) {
    if (lightId < 0) return 0.0;

    Light light = lightBuffer.lights[lightId];
    if (light.selectionProbability <= 0.0) return 0.0;

    float cosLight = abs(dot(normal, wi));
    return light.selectionProbability * dist*dist / (max(cosLight, EPS) * light.area);
}

LightSample sampleLight(in Hit hit, inout RngState rng) {
    LightSample light;
    light.skip = false;
    int lightId = getRandomLightId(rng);
    if (lightId < 0) {
        light.pdf = -1.0;
        return light;
    }

    Object lightObj = objectBuffer.objects[lightBuffer.lights[lightId].objectId];
    SurfaceSample surfaceSample = sampleSurface(lightObj, lightBuffer.lights[lightId].area, rng);

    vec3 toLight = surfaceSample.p - hit.p;
    float dist = length(toLight);
    light.wi = toLight / dist;

    // Reject back-facing samples: the sampled face is not visible from the shading point.
    if (dot(surfaceSample.normal, light.wi) >= 0.0) {
        light.pdf = -1.0;
        return light;
    }

    Statistics shadowStats = Statistics(0, 0);
    vec3 shadowOrigin = hit.p + hit.normal * EPS;

    Hit shadowHit = intersection(Ray(shadowOrigin, light.wi), false, dist - EPS, shadowStats);
    if (foundIntersection(shadowHit) &&
        !(shadowHit.object.id == lightObj.id && shadowHit.object.type == lightObj.type)) {
        Material shadowMat = getMaterial(shadowHit.object);
        if (shadowMat.type == mat_Volume || isTransmissive(unpackMaterial(shadowMat))) {
            light.skip = true;
        }
        light.pdf = -1.0;
        return light;
    }

    light.pdf = lightPDF(lightId, dist, surfaceSample.normal, light.wi);
    ResolvedMaterial lightMat = unpackMaterial(getMaterial(lightObj));
    light.Le = albedo(lightMat) * emissiveEmissionStrength(lightMat);
    return light;
}

#endif
