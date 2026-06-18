#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

#include "inputs.glsl"
#include "utils.glsl"
#include "materials/materials.glsl"
#include "objects.glsl"
#include "lights.glsl"
#include "global.glsl"
#include "random.glsl"

layout(rgba32f, set = 0, binding = 13) writeonly uniform image2D outputImage;


Ray getRay(Camera camera, vec2 ndc_pos, in bool enableFocus, inout uint seed) {
    vec3 forward = normalize(camera.dir);
    vec3 right   = normalize(cross(forward, camera.up));
    vec3 up      = cross(right, forward);

    float scr_x = ndc_pos.x * 0.5f + 0.5f;
    float scr_y = ndc_pos.y * 0.5f + 0.5f;

    float cam_x = (2.f * scr_x - 1.f) * ubo.aspect * ubo.tanHFov;
    float cam_y = (1.f - 2.f * scr_y) * ubo.tanHFov;

    vec3 offset = vec3(0.0);
    if (enableFocus) {
        vec2 p = randomInDisk(seed);
        float lens_r = ubo.aperture * 0.5;
        offset = lens_r * (right * p.x + up * p.y);
    }

    vec3 origin = camera.pos + offset;

    vec3 target = camera.pos + (cam_x * right + cam_y * up + forward) * ubo.focusDepth;
    vec3 dir = normalize(target - origin);

    return Ray(origin, dir);
}

Hit intersection(in Ray ray) {
    Hit bestHit = NO_HIT;

    int hitChecks = 0;
    for (int i = 0; i < objectBuffer.objectCount; i++) {
        Hit hit = rayObjectIntersection(ray, objectBuffer.objects[i]);
        hitChecks += hit.hitChecks;
        if (foundIntersection(hit) && hit.t < bestHit.t) {
            bestHit = hit;
        }
    }

    bestHit.hitChecks = hitChecks;
    return bestHit;
}

vec3 skyColor(vec3 dir) {
    float t = clamp(0.5*(dir.y + 1.0), 0.0, 1.0);
    vec3 zenith, horizon;

    switch (ubo.lightMode) {
        case lightMode_Day:
            zenith = vec3(0.5, 0.7, 1.0);
            horizon = vec3(1.0, 1.0, 1.0);
            break;
        case lightMode_Sunset:
            zenith = vec3(0.2, 0.1, 0.4);
            horizon = vec3(1.0, 0.4, 0.2);
            break;
        case lightMode_Night:
            zenith  = vec3(0.01, 0.01, 0.03);
            horizon = vec3(0.05, 0.05, 0.1);
            break;
        case lightMode_Empty:
            return vec3(0.0);
            break;
        default:
            return vec3(1.0, 0.0, 1.0);
            break;
    }

    return mix(horizon, zenith, t);
}

vec3 traceRay(in Camera camera, in Ray ray, inout uint seed, inout PixelInfo pixelInfo) {
    Hit hit = intersection(ray);

    if (foundIntersection(hit)) {
        Material diffuseMat = resolveMaterial(getMaterial(hit.object), hit);

        vec3 currNormal = normalize(hit.normal);
        vec3 currPosition = hit.p;
        vec3 currDiffuse = diffuseMat.albedo;

        float guideMix = 1.0 / float(pixelInfo.count);

        if (pixelInfo.normal.w > 0.0) {
            vec3 avgNormal = mix(pixelInfo.normal.xyz, currNormal, guideMix);
            pixelInfo.normal = vec4(normalize(avgNormal), 1.0);
        } else {
            pixelInfo.normal = vec4(currNormal, 1.0);
        }

        if (pixelInfo.position.w > 0.0) {
            pixelInfo.position = vec4(mix(pixelInfo.position.xyz, currPosition, guideMix), 1.0);
        } else {
            pixelInfo.position = vec4(currPosition, 1.0);
        }

        if (pixelInfo.diffuse.w > 0.0) {
            pixelInfo.diffuse = vec4(mix(pixelInfo.diffuse.xyz, currDiffuse, guideMix), 1.0);
        } else {
            pixelInfo.diffuse = vec4(currDiffuse, 1.0);
        }
    }

    if (ubo.debugView == debug_HitChecks) {
        float v = hit.hitChecks / 64.0;
        v = clamp(v, 0, 1);
        return vec3(v);
    }

    vec3 throughput = vec3(1.0);
    vec3 radiance = vec3(0.0);

    int i = 0;
    BSDFSample bsdf;
    LightSample lightSample;
    Material mat;

    BSDFSample prevBsdf;
    Hit prevHit;
    bool insideMedium = false;
    vec3 mediumAbsorption = vec3(1.0f);

    for (; i < ubo.maxBounces; i++) {
        if (foundIntersection(hit)) {
            mat = getMaterial(hit.object);

            if (insideMedium) {
                throughput *= pow(max(mediumAbsorption, vec3(1e-4)), vec3(hit.t));
            }
            if (mat.type == mat_Emissive) {
                if (i == 0) {
                    radiance = mat.albedo;
                    break;
                }

                vec3 Le = mat.albedo * mat.emissionStrength;
                float w = 1.0;
                if (ubo.importanceSampling == 1 && !prevBsdf.isDelta) {
                    float pdfL = lightPDF(prevHit.object.id, length(prevHit.p - hit.p), prevHit.normal, prevBsdf.wi, hit.p - prevHit.p);
                    float pdfB = prevBsdf.pdf;
                    float denom = pdfB*pdfB + pdfL*pdfL;
                    w = (denom > 0.0) ? (pdfB*pdfB / denom) : 1.0;
                }

                radiance += clamp(throughput * w, 0.0, 1.5) * Le;
                break;
            }
            if (mat.type == mat_Dielectric) {
                if (hit.frontFace) {
                    insideMedium = true;
                    mediumAbsorption = mat.albedo;
                } else if (dot(prevBsdf.wi, hit.normal) < 0.0) {
                    insideMedium = false;
                }
            }
 
            bsdf = sampleBSDF(mat, hit, -ray.dir, seed);
            if (bsdf.pdf < EPS && !bsdf.isDelta) {
                break;
            }
            if (ubo.importanceSampling == 1 && !bsdf.isDelta) {
                lightSample = sampleLight(hit, seed);
                BSDFEval eval = evalBSDF(mat, hit, -ray.dir, lightSample.wi);
                if (lightSample.pdf > EPS) {
                    float cosTheta = max(dot(hit.normal, lightSample.wi), 0.0);
                    float wMIS = (lightSample.pdf*lightSample.pdf) / (lightSample.pdf*lightSample.pdf + eval.pdf*eval.pdf);
                    radiance += clamp(throughput * eval.f * cosTheta * lightSample.Le * wMIS / lightSample.pdf, 0.0, 1.5);
                }
            }

            throughput *= bsdf.weight;

            // Russian Roulette
            if (i >= 2) {
                float p = clamp(luma(throughput), 0.1, 1.0);
                if (rand(seed) > p) break;
                throughput /= p;
            }
            prevBsdf = bsdf;
            prevHit = hit;

            ray = Ray(hit.p + bsdf.wi * EPS, bsdf.wi);
            hit = intersection(ray);
        } else {
            radiance += throughput * skyColor(ray.dir);
            break;
        }
    }
    if (i == ubo.maxBounces) radiance = vec3(0.0);

    if (ubo.debugView == debug_Bounces) {
        return vec3(i / float(ubo.maxBounces));
    }
    return radiance;
}

uint varianceIndexFromCoord(ivec2 coord, ivec2 texSize) {
    return uint(coord.y * texSize.x + coord.x);
}

ivec2 blockCoordFromResolution(ivec2 pixelCoord, vec2 screenCoord, ivec2 texSize, float resolution) {
    ivec2 blockCoord = pixelCoord;
    if (resolution > 1.0f) {
        blockCoord = ivec2(floor(screenCoord / resolution) * resolution);
    }
    return clamp(blockCoord, ivec2(0), texSize - ivec2(1));
}

void updateVariance(in float value, inout PixelInfo pixelInfo) {
    float delta = value - pixelInfo.mean;
    pixelInfo.mean += delta / pixelInfo.count;
    float delta2 = value - pixelInfo.mean;
    pixelInfo.m2 += delta * delta2;
}

float computeSpatialVariance(ivec2 blockCoord, vec2 texSize) {
    const int radius = 2;
    float mean = 0.0;
    float meanSq = 0.0;
    int samples = 0;
    float stepSize = max(ubo.resolution, 1.0);
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            ivec2 p = blockCoord + ivec2(x, y) * int(stepSize);
            p = clamp(p, ivec2(0), ivec2(texSize) - ivec2(1));
            vec3 c = texelFetch(prevTex, p, 0).rgb;
            float lum = luma(c);
            mean += lum;
            meanSq += lum * lum;
            samples++;
        }
    }
    mean /= float(samples);
    meanSq /= float(samples);
    return sqrt(max(meanSq - mean * mean, 0.0));
}

float computeSampleProbability(inout PixelInfo pixelInfo, ivec2 blockCoord, ivec2 texSize, float resolution) {
    float temporalVariance = (pixelInfo.count > 1.0) ? (pixelInfo.m2 / (pixelInfo.count - 1.0)) : 0.0;
    float spatialSigma = computeSpatialVariance(blockCoord, texSize);

    float sigma = mix(sqrt(max(temporalVariance, 0.0)), spatialSigma, 0.5);
    float minAdaptiveSamples = max(float(ubo.varianceWarmupSamples), 0.0);
    float proba = clamp(sigma * 4.0, 0.1, 1.0);
    pixelInfo.varianceProba = proba;
    return (pixelInfo.count < minAdaptiveSamples) ? 1.0 : proba;
}

vec3 computeFragmentColor(in Camera camera, in vec2 fragPos, inout uint seed, float sampleProb, inout PixelInfo pixelInfo, out float takenSamples) {
    vec3 colorSum = vec3(0);
    takenSamples = 0.0;
    for (int i = 0; i < ubo.samplesPerPixel; i++) {
        if (sampleProb >= 1.0 || rand(seed) <= sampleProb) {
            pixelInfo.count += 1;
            vec2 offset = ubo.resolution * vec2(rand(seed), rand(seed)) / ubo.screenSize;
            Ray ray = getRay(camera, fragPos + offset, true, seed);
            vec3 rayColor = traceRay(camera, ray, seed, pixelInfo);
            if (isnan(rayColor.r) || isnan(rayColor.g) || isnan(rayColor.b)) {
                pixelInfo.count -= 1;
                continue;
            }
            colorSum.rgb += rayColor.rgb;

            takenSamples += 1.0;
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

    float prevScale = max(ubo.prevResolution, 1.0);
    ivec2 prevBlockCoord = pixelCoord;
    if (prevScale > 1.0) {
        prevBlockCoord = ivec2(floor(screenCoord / prevScale) * prevScale);
        prevBlockCoord = clamp(prevBlockCoord, ivec2(0), texSize - ivec2(1));
    }

    vec3 prevColor = texelFetch(prevTex, prevBlockCoord, 0).rgb;
    if (ubo.frameCount <= 1) {
        prevColor = vec3(0);

        uint varianceIndex = varianceIndexFromCoord(pixelCoord, texSize);
        PixelInfo initInfo = pixelInfoBuffer.pixels[varianceIndex];
        initInfo.normal.w = 0.0;
        initInfo.position.w = 0.0;
        initInfo.diffuse.w = 0.0;
        initInfo.mean = 0.0;
        initInfo.m2 = 0.0;
        initInfo.count = 0;
        initInfo.varianceProba = 0.0;
        initInfo.selectionMask = 0;
        pixelInfoBuffer.pixels[varianceIndex] = initInfo;
    }

    Camera camera = Camera(ubo.cameraPos, ubo.cameraDir, vec3(0, 1, 0));
    uint seed = initSeed(uvec2(pixelCoord), uint(ubo.frameCount));

    ivec2 blockCoord = blockCoordFromResolution(pixelCoord, screenCoord, texSize, ubo.resolution);
    uint blockVarianceIndex = varianceIndexFromCoord(blockCoord, texSize);
    PixelInfo pixelInfo = pixelInfoBuffer.pixels[blockVarianceIndex];
    float prevCount = max(pixelInfo.count, 0.0);

    float sampleProb = 1.0;
    if (ubo.varianceSampling != 0)
        sampleProb = computeSampleProbability(pixelInfo, blockCoord, texSize, ubo.prevResolution);

    float takenSamples = 0.0;
    vec3 colorSum = vec3(0);
    bool isCenterPixel = ubo.resolution == 1.0f || pixelCoord == blockCoord;
    if (isCenterPixel) {
        colorSum = computeFragmentColor(camera, fragPos, seed, sampleProb, pixelInfo, takenSamples);
        PixelInfo updatedInfo = pixelInfoBuffer.pixels[blockVarianceIndex];
        updatedInfo.normal = pixelInfo.normal;
        updatedInfo.position = pixelInfo.position;
        updatedInfo.diffuse = pixelInfo.diffuse;
        updatedInfo.mean = pixelInfo.mean;
        updatedInfo.m2 = pixelInfo.m2;
        updatedInfo.count = pixelInfo.count;
        updatedInfo.varianceProba = pixelInfo.varianceProba;
        pixelInfoBuffer.pixels[blockVarianceIndex] = updatedInfo;
    }

    if (ubo.resolution < ubo.prevResolution) prevColor = colorSum / max(takenSamples, 1.0);

    Ray primaryRay = getRay(camera, fragPos, false, seed);
    Hit selectedHit = NO_HIT;
    int selectedIntersection = 0;
    if (objectBuffer.selectedObjectId >= 0) {
        selectedHit = rayObjectIntersection(primaryRay, objectBuffer.objects[objectBuffer.selectedObjectId]);
        if (foundIntersection(selectedHit)) {
            selectedIntersection = 1;
        }
    }
    uint selectionIndex = varianceIndexFromCoord(pixelCoord, texSize);
    PixelInfo selectionInfo = pixelInfoBuffer.pixels[selectionIndex];
    selectionInfo.selectionMask = selectedIntersection;
    pixelInfoBuffer.pixels[selectionIndex] = selectionInfo;

    float totalCount = prevCount + takenSamples;
    vec3 mixedColor = prevColor;
    if (totalCount > 0.0) {
        mixedColor = (prevColor * prevCount + colorSum) / totalCount;
    }
    imageStore(outputImage, pixelCoord, vec4(mixedColor, 1.0));
}
