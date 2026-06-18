#ifndef LIGHT_GLSL
#define LIGHT_GLSL

#include "inputs.glsl"
#include "global.glsl"
#include "random.glsl"

int getRandomLightId(inout uint seed) {
    if (lightBuffer.totalArea <= EPS) return -1;

    float r = rand(seed);
    float t = 0.0;
    int i = 0;
    while (r > t) {
        t += lightBuffer.lights[i].area / lightBuffer.totalArea;
        i++;
    }
    return i-1;
}

// TODO: store the light count or find a better way to find the corresponding light from an object
uint getLightIdFromObjectId(uint objectId) {
    uint i = 0;
    for (float a = 0; a < lightBuffer.totalArea;) {
        if (lightBuffer.lights[i].objectId == objectId) return i;
        a += lightBuffer.lights[i].area;
        i++;
    }
    return 0;
}

Hit intersection(in Ray ray); // Forward declaration

struct LightSample {
    vec3 wi;
    float pdf;
    vec3 Le;
};

float lightPDF(in uint objectId, in float dist, in vec3 normal, in vec3 wo, in vec3 wi) {
    uint lightId = getLightIdFromObjectId(objectId);
    if (lightId < 0) return -1.0;

    Light light = lightBuffer.lights[lightId];
    float cosLight = max(dot(normal, wi), 0.0);
    return light.pdfA * dist*dist / max(cosLight, EPS) * light.area / lightBuffer.totalArea;
}

LightSample sampleLight(in Hit hit, inout uint seed) {
    LightSample light;
    int lightId = getRandomLightId(seed);
    if (lightId < 0) {
        light.pdf = -1.0;
        return light;
    }

    Object lightObj = objectBuffer.objects[lightBuffer.lights[lightId].objectId];
    SurfaceSample surfaceSample = sampleSurface(lightObj, lightBuffer.lights[lightId].area, seed);

    vec3 toLight = surfaceSample.p - hit.p;
    light.wi = normalize(toLight);

    Ray shadowRay = Ray(hit.p + hit.normal * EPS, light.wi);
    Hit shadowHit = intersection(shadowRay);
    bool visible = foundIntersection(shadowHit) && (shadowHit.object.id == lightObj.id && shadowHit.object.type == lightObj.type);
    if (!visible) {
        light.pdf = -1.0;
        return light;
    }

    float dist = length(toLight);
    light.pdf = lightPDF(lightObj.id, dist, surfaceSample.normal, shadowRay.dir, light.wi);

    Material lightMat = getMaterial(lightObj);
    light.Le = lightMat.albedo * lightMat.emissionStrength;
    return light;
}

#endif