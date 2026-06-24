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

Hit intersection(in Ray ray, bool anyHit, float tMax, inout Statistics stats); // Forward declaration

struct LightSample {
    vec3 wi;
    float pdf;
    vec3 Le;
};

float lightPDF(in uint objectId, in float dist, in vec3 normal, in vec3 wo, in vec3 wi) {
    uint lightId = getLightIdFromObjectId(objectId);
    if (lightId < 0) return -1.0;

    Light light = lightBuffer.lights[lightId];
    float cosLight = abs(dot(normal, wi));
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
    float dist = length(toLight);
    light.wi = toLight / dist;

    // Reject back-facing samples: the sampled face is not visible from the shading point.
    if (dot(surfaceSample.normal, light.wi) >= 0.0) {
        light.pdf = -1.0;
        return light;
    }

    Statistics shadowStats = Statistics(0, 0);
    vec3 shadowOrigin = hit.p + hit.normal * EPS;
    float remaining = dist - EPS;
    vec3 transmission = vec3(1.0);

    for (int i = 0; i < 8 && remaining > 0.0; i++) {
        Hit shadowHit = intersection(Ray(shadowOrigin, light.wi), true, remaining, shadowStats);
        if (!foundIntersection(shadowHit)) break;

        // Reached the light object itself
        if (shadowHit.object.id == lightObj.id && shadowHit.object.type == lightObj.type) break;

        Material shadowMat = getMaterial(shadowHit.object);
        float T = shadowTransmissionFactor(shadowMat, shadowHit, light.wi);
        if (T < 0.0) {
            light.pdf = -1.0;
            return light;
        }

        transmission *= T;
        if (shadowMat.density > EPS) {
            transmission *= pow(max(shadowMat.albedo, vec3(1e-4)), vec3(shadowHit.t * shadowMat.density));
        }

        remaining -= shadowHit.t + EPS;
        shadowOrigin = shadowHit.p + light.wi * EPS;
    }

    if (max(transmission.r, max(transmission.g, transmission.b)) < EPS) {
        light.pdf = -1.0;
        return light;
    }

    light.pdf = lightPDF(lightObj.id, dist, surfaceSample.normal, light.wi, light.wi);
    Material lightMat = getMaterial(lightObj);
    light.Le = lightMat.albedo * lightMat.emissionStrength * transmission;
    return light;
}

#endif