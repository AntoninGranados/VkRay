#ifndef LIGHT_GLSL
#define LIGHT_GLSL

#include "inputs.glsl"
#include "global.glsl"
#include "random.glsl"

int getLightId(inout vec3 seed) {
    if (lightBuffer.totalArea == 0) return -1;

    float r = rand(seed);
    float t = 0.0;
    int i = 0;
    while (r > t) {
        t += lightBuffer.lights[i].area / lightBuffer.totalArea;
        i++;
    }
    return i-1;
}

Hit intersection(in Ray ray); // Forward declaration

vec3 importanceSampleLight(in Material surfaceMat, in Hit hit, in ScatterResult scatterResult, inout vec3 seed) {
    if (ubo.importanceSampling != 1 || !scatterResult.isDiffuse) return vec3(0.0);

    int lightId = getLightId(seed);
    if (lightId < 0) return vec3(0.0);

    Object lightObj = objectBuffer.objects[lightBuffer.lights[lightId].objectId];

    SurfaceSample surfaceSample = sampleSurface(lightObj, lightBuffer.lights[lightId].area, seed);

    vec3 toLight = surfaceSample.p - scatterResult.scattered.origin;
    float dist2 = dot(toLight, toLight);
    float dist = sqrt(dist2);
    vec3 toLightDir = toLight / dist;

    float cosSurface = max(dot(hit.normal, toLightDir), 0.0);
    float cosLight = max(dot(-toLightDir, surfaceSample.normal), 0.0);
    if (cosSurface <= 0.0 || cosLight <= 0.0) return vec3(0.0);

    Ray shadowRay = Ray(scatterResult.scattered.origin, toLightDir);
    Hit shadowHit = intersection(shadowRay);
    bool visible = foundIntersection(shadowHit) && shadowHit.t >= dist - EPS;
    if (!visible) return vec3(0.0);

    float pdfW = lightBuffer.lights[lightId].pdfA * dist2 / max(cosLight, EPS);

    Material lightMat = getMaterial(lightObj);
    vec3 Le = lightMat.albedo * emissiveIntensity(lightMat);
    return (surfaceMat.albedo / PI) * Le * cosSurface / max(pdfW, EPS);
}

#endif
