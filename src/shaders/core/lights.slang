import "scene.slang";
import "global.slang";
import "random/utils.slang";

ByteAddressBuffer lightBuffer;

uint lightCount() {
    return lightBuffer.Load<uint>(0);
}

Light loadLight(uint index) {
    return lightBuffer.Load<Light>(sizeof(uint) + index * sizeof(Light));
}

int getRandomLightId(inout RngState rng) {
    if (lightCount() == 0) return -1;

    float r = rand(rng);
    int low = 0;
    int high = int(lightCount()) - 1;
    while (low < high) {
        int middle = (low + high) / 2;
        if (loadLight(middle).cumulativeProbability > r) high = middle;
        else low = middle + 1;
    }
    return low;
}

struct LightSample {
    float3 wi;
    float pdf;
    float3 Le;
    bool skip;
};

float lightPDF(in int lightId, in float dist, in float3 normal, in float3 wi) {
    if (lightId < 0) return 0.0;

    Light light = loadLight(lightId);
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

    Object lightObj = loadObject(loadLight(lightId).objectId);
    SurfaceSample surfaceSample = sampleSurface(lightObj, loadLight(lightId).area, rng);

    float3 toLight = surfaceSample.p - hit.p;
    float dist = length(toLight);
    light.wi = toLight / dist;

    // Reject back-facing samples: the sampled face is not visible from the shading point.
    if (dot(surfaceSample.normal, light.wi) >= 0.0) {
        light.pdf = -1.0;
        return light;
    }

    Statistics shadowStats = Statistics(0, 0);
    float3 shadowOrigin = hit.p + hit.normal * EPS;

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
    light.Le = lightMat.albedo * lightMat.emissiveEmissionStrength;
    return light;
}
