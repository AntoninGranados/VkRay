#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

#include "inputs.glsl"
#include "utils.glsl"
#include "materials/materials.glsl"
#include "objects.glsl"
#include "lights.glsl"
#include "global.glsl"
#include "random.glsl"
#include "sky.glsl"
#include "adaptive_sampling.glsl"
#include "camera.glsl"

layout(rgba32f, set = 0, binding = 14) writeonly uniform image2D outputImage;

Hit intersection(in Ray ray, bool anyHit, float tMax, inout Statistics stats) {
    Hit bestHit = NO_HIT;
    for (int i = 0; i < objectBuffer.objectCount; i++) {
        Hit hit = rayObjectIntersection(ray, objectBuffer.objects[i], anyHit, tMax, stats);
        if (foundIntersection(hit) && hit.t < tMax) {
            if (anyHit) return hit;
            if (hit.t < bestHit.t) bestHit = hit;
        }
    }
    return bestHit;
}

void collectGBuffer(in Ray ray, in Camera camera, inout PixelInfo pixelInfo) {
    Statistics dummy = Statistics(0u, 0u);
    Hit firstHit = intersection(ray, false, INFINITY, dummy);

    if (!foundIntersection(firstHit)) {
        pixelInfo.aov.skyMask = 1u;
        return;
    }

    Material mat = resolveMaterial(getMaterial(firstHit.object), firstHit);
    mat_setAlbedo(mat, mat_albedo(mat) * firstHit.vertexColor);
    float guideMix = 1.0 / float(pixelInfo.count);

    vec3 right = normalize(camera.U);
    vec3 up    = normalize(camera.V);

    vec3 normalW = normalize(firstHit.normal);
    vec3 positionW = firstHit.p;
    vec3 albedo = mat_albedo(mat);
    float roughness = (mat.type == mat_Metal || mat.type == mat_Glossy || mat.type == mat_Dielectric || mat.type == mat_Principled)
                      ? mat.payload[3] : 1.0;
    vec3 hitOff = positionW - camera.eye;
    vec3 camDir = camera.W;
    vec2 normal = vec2(dot(normalW, right), dot(normalW, up));
    vec3 position = vec3(dot(hitOff, right), dot(hitOff, up), dot(hitOff, camDir));

    if (pixelInfo.aov.hitValid != 0u) {
        pixelInfo.aov.positionW = mix(pixelInfo.aov.positionW, positionW, guideMix);
        pixelInfo.aov.position  = mix(pixelInfo.aov.position,  position,  guideMix);
        pixelInfo.aov.normalW   = normalize(mix(pixelInfo.aov.normalW, normalW, guideMix));
        pixelInfo.aov.normal    = mix(pixelInfo.aov.normal,    normal,    guideMix);
        pixelInfo.aov.albedo    = mix(pixelInfo.aov.albedo,    albedo,    guideMix);
        pixelInfo.aov.roughness = mix(pixelInfo.aov.roughness, roughness, guideMix);
    } else {
        pixelInfo.aov.positionW = positionW;
        pixelInfo.aov.position  = position;
        pixelInfo.aov.normalW   = normalW;
        pixelInfo.aov.normal    = normal;
        pixelInfo.aov.albedo    = albedo;
        pixelInfo.aov.roughness = roughness;
        pixelInfo.aov.matType   = uint(mat.type);
        pixelInfo.aov.hitValid  = 1u;
    }
}

vec3 traceRay(in Camera camera, in Ray ray, inout uint seed, inout PixelInfo pixelInfo) {
    Statistics stats = Statistics(0, 0);
    Hit hit = intersection(ray, false, INFINITY, stats);

    pixelInfo.bvhChecks      = stats.bvhChecks;
    pixelInfo.triangleChecks = stats.triangleChecks;

    vec3 throughput = vec3(1.0);
    vec3 radiance = vec3(0.0);

    int i = 0;
    BSDFSample bsdf;
    LightSample lightSample;
    Material mat;

    BSDFSample prevBsdf = BSDFSample(
        vec3(0.0),
        vec3(0.0),
        0.0,
        false,
        BSDFMediumInfo(false, false, vec3(1.0), 1.0, 0.0, 0.0)
    );
    Hit prevHit;
    BSDFMediumInfo currentMedium = BSDFMediumInfo(false, false, vec3(1.0), 0.0, 0.0, 0.0);
    bool prevIsSkipped = false;

    for (; i < ubo.render.maxBounces; i++) {
        if (!foundIntersection(hit)) {
            radiance += throughput * skyColor(ray.dir);
            break;
        }

        if (currentMedium.isVolume) {
            float sigma_t = currentMedium.density;
            float omega   = currentMedium.scatterAlbedo;
            float sigma_s = sigma_t * omega;
            float sigma_a = sigma_t * (1.0 - omega);

            float t_scatter = (sigma_s > EPS) ? (-log(rand(seed)) / sigma_s) : INFINITY;
            if (t_scatter < hit.t) {
                ray.origin += ray.dir * t_scatter;

                throughput *= pow(max(currentMedium.absorption, vec3(1e-4)), vec3(sigma_a * t_scatter));

                if (ubo.render.importanceSampling == 1) {
                    Hit scatterHit = Hit(ray.origin, vec3(0.0), t_scatter, true, OBJECT_NONE, vec3(1.0));
                    LightSample volLight = sampleLight(scatterHit, seed);
                    prevIsSkipped = volLight.skip;
                    if (volLight.pdf > EPS) {
                        float phase = phaseFunctionHG(currentMedium.anisotropic, volLight.wi, ray.dir);
                        float wMIS = powerHeuristic(volLight.pdf, phase);
                        radiance += throughput * omega * currentMedium.absorption * phase * volLight.Le * wMIS / volLight.pdf;
                    }
                } else {
                    prevIsSkipped = false;
                }

                vec3 incomingDir = ray.dir;
                ray.dir = sampleHG(currentMedium.anisotropic, ray.dir, seed);
                throughput *= omega * currentMedium.absorption;

                prevBsdf.pdf = phaseFunctionHG(currentMedium.anisotropic, ray.dir, incomingDir);
                prevBsdf.isDelta = false;
                prevHit.p = ray.origin;

                hit = intersection(ray, false, INFINITY, stats);
                continue;
            } else {
                throughput *= pow(max(currentMedium.absorption, vec3(1e-4)), vec3(sigma_a * hit.t));
            }
        }

        mat = getMaterial(hit.object);
        mat_setAlbedo(mat, mat_albedo(mat) * hit.vertexColor);

        if (mat.type == mat_Emissive) {
            if (i == 0) {
                radiance = mat_albedo(mat) * mat_emissive_emissionStrength(mat);
                break;
            }

            vec3 Le = mat_albedo(mat) * mat_emissive_emissionStrength(mat);
            float w = 1.0;
            if (ubo.render.importanceSampling == 1 && !prevBsdf.isDelta && !prevIsSkipped) {
                float pdfL = lightPDF(hit.object.id, length(hit.p - prevHit.p), hit.normal, prevBsdf.wi, prevHit.p - hit.p);
                w = powerHeuristic(prevBsdf.pdf, pdfL);
            }

            radiance += throughput * w * Le;
            break;
        }

        bsdf = sampleBSDF(mat, hit, -ray.dir, seed);
        if (bsdf.pdf < EPS && !bsdf.isDelta) {
            break;
        }
        if (mat.type == mat_Volume || mat.type == mat_Dielectric || mat.type == mat_Principled) {
            currentMedium = bsdf.medium.isVolume ? bsdf.medium : BSDFMediumInfo(false, false, vec3(1.0), 0.0, 0.0, 0.0);
        } else {
            currentMedium = BSDFMediumInfo(false, false, vec3(1.0), 0.0, 0.0, 0.0);
        }
        bool isSkipped = false;
        if (ubo.render.importanceSampling == 1 && !bsdf.isDelta) {
            lightSample = sampleLight(hit, seed);
            isSkipped = lightSample.skip;
            if (lightSample.pdf > EPS) {
                BSDFEval eval = evalBSDF(mat, hit, -ray.dir, lightSample.wi);
                float cosTheta = max(dot(hit.normal, lightSample.wi), 0.0);
                float wMIS = powerHeuristic(lightSample.pdf, eval.pdf);
                radiance += throughput * eval.f * cosTheta * lightSample.Le * wMIS / lightSample.pdf;
            }
        }

        throughput *= bsdf.weight;

        // Russian Roulette
        if (i >= 2 && !bsdf.isDelta) {
            float p = clamp(luma(throughput), 0.1, 1.0);
            if (rand(seed) > p) break;
            throughput /= p;
        }
        if (mat.type != mat_Volume && !((mat.type == mat_Dielectric || mat.type == mat_Principled) && bsdf.medium.isVolume)) {
            prevBsdf = bsdf;
            prevHit = hit;
            prevIsSkipped = isSkipped;
        } else {
            prevIsSkipped = true;
        }

        ray = Ray(hit.p + bsdf.wi * EPS, bsdf.wi);
        hit = intersection(ray, false, INFINITY, stats);
        if (prevBsdf.medium.isDielectric && !prevBsdf.medium.isVolume) {
            throughput *= pow(max(prevBsdf.medium.absorption, vec3(1e-4)), vec3(hit.t * prevBsdf.medium.density));
        }
    }
    if (i == ubo.render.maxBounces) radiance = vec3(0.0);

    pixelInfo.bounces = uint(i);
    return radiance;
}

vec3 computeFragmentColor(in Camera camera, in vec2 fragPos, inout uint seed, float sampleProb, inout PixelInfo pixelInfo, out float takenSamples) {
    vec3 colorSum = vec3(0);
    takenSamples = 0.0;
    if (sampleProb >= 1.0 || rand(seed) <= sampleProb) {
        pixelInfo.count++;
        vec2 offset = vec2(rand(seed), rand(seed)) / ubo.screen.size;
        Ray ray = getRay(camera, fragPos + offset, seed);
        collectGBuffer(ray, camera, pixelInfo);
        vec3 rayColor = traceRay(camera, ray, seed, pixelInfo);
        if (isnan(rayColor.r) || isnan(rayColor.g) || isnan(rayColor.b) || isinf(rayColor.r) || isinf(rayColor.g) || isinf(rayColor.b)) {
            pixelInfo.count--;
        } else {
            if (ubo.render.clipAccumulation == 1) rayColor = min(rayColor, vec3(ubo.render.clipThreshold));
            colorSum.rgb += rayColor.rgb;

            takenSamples = 1.0;
            updateVariance(luma(rayColor.rgb), pixelInfo);
        }
    }

    return colorSum;
}

void main() {
    ivec2 texSize = textureSize(prevTex, 0);
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);

    if (pixelCoord.x >= texSize.x || pixelCoord.y >= texSize.y) return;

    vec2 screenCoord = vec2(pixelCoord) + vec2(0.5);
    vec2 uv = screenCoord / vec2(texSize);
    vec2 fragPos = uv * 2.0 - 1.0;

    vec3 prevColor = texelFetch(prevTex, pixelCoord, 0).rgb;
    if (ubo.sampleCount <= 1) {
        prevColor = vec3(0);

        uint varianceIndex = varianceIndexFromCoord(pixelCoord, texSize);
        PixelInfo initInfo = pixelInfoBuffer.pixels[varianceIndex];
        initInfo.aov.hitValid = 0u;
        initInfo.aov.matType  = 0u;
        initInfo.aov.skyMask  = 0u;
        initInfo.mean                   = 0.0;
        initInfo.m2                     = 0.0;
        initInfo.count                  = 0;
        initInfo.bounces                = 0u;
        initInfo.bvhChecks              = 0u;
        initInfo.triangleChecks         = 0u;
        initInfo.varianceProba          = 0.0;
        pixelInfoBuffer.pixels[varianceIndex] = initInfo;
    }

    Camera camera = Camera(ubo.camera.eye, ubo.camera.U, ubo.camera.V, ubo.camera.W);
    uint seed = initSeed(uvec2(pixelCoord), uint(ubo.sampleCount));

    uint blockVarianceIndex = varianceIndexFromCoord(pixelCoord, texSize);
    PixelInfo pixelInfo = pixelInfoBuffer.pixels[blockVarianceIndex];
    float prevCount = max(pixelInfo.count, 0.0);

    float sampleProb = 1.0;
    if (ubo.render.varianceSampling != 0)
        sampleProb = computeSampleProbability(pixelInfo, pixelCoord, texSize);

    float takenSamples = 0.0;
    vec3 colorSum = computeFragmentColor(camera, fragPos, seed, sampleProb, pixelInfo, takenSamples);
    PixelInfo updatedInfo = pixelInfoBuffer.pixels[blockVarianceIndex];
    updatedInfo.aov             = pixelInfo.aov;
    updatedInfo.mean            = pixelInfo.mean;
    updatedInfo.m2              = pixelInfo.m2;
    updatedInfo.count           = pixelInfo.count;
    updatedInfo.bounces         = pixelInfo.bounces;
    updatedInfo.bvhChecks       = pixelInfo.bvhChecks;
    updatedInfo.triangleChecks  = pixelInfo.triangleChecks;
    updatedInfo.varianceProba   = pixelInfo.varianceProba;
    pixelInfoBuffer.pixels[blockVarianceIndex] = updatedInfo;

    float totalCount = prevCount + takenSamples;
    vec3 mixedColor = prevColor;
    if (totalCount > 0.0) {
        mixedColor = (prevColor * prevCount + colorSum) / totalCount;
    }
    imageStore(outputImage, pixelCoord, vec4(mixedColor, 1.0));
}
