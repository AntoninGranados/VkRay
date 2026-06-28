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

layout(rgba32f, set = 0, binding = 14) writeonly uniform image2D outputImage;


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

vec3 traceRay(in Camera camera, in Ray ray, inout uint seed, inout PixelInfo pixelInfo) {
    Statistics stats = Statistics(0, 0);
    Hit hit = intersection(ray, false, INFINITY, stats);

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
        float bvh = float(stats.bvhChecks) / 256.0;
        float tri = float(stats.triangleChecks) / 256.0;
        if (max(bvh, tri) > 1.0) return vec3(1.0);
        return vec3(tri, 0.0, bvh);
    }

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
        BSDFMediumInfo(false, false, vec3(1.0), 1.0, 0.0)
    );
    Hit prevHit;
    BSDFMediumInfo currentMedium = BSDFMediumInfo(false, false, vec3(1.0), 0.0, 0.0);

    for (; i < ubo.maxBounces; i++) {
        if (foundIntersection(hit)) {
            mat = getMaterial(hit.object);

            if (mat.emissionStrength > 0) {
                if (i == 0) {
                    radiance = mat.albedo * mat.emissionStrength;
                    break;
                }

                vec3 Le = mat.albedo * mat.emissionStrength;
                float w = 1.0;
                if (ubo.importanceSampling == 1 && !prevBsdf.isDelta) {
                    float pdfL = lightPDF(hit.object.id, length(hit.p - prevHit.p), hit.normal, prevBsdf.wi, prevHit.p - hit.p);
                    float pdfB = prevBsdf.pdf;
                    float denom = pdfB*pdfB + pdfL*pdfL;
                    w = (denom > 0.0) ? (pdfB*pdfB / denom) : 1.0;
                }

                radiance += throughput * w * Le;
                break;
            }

            bsdf = sampleBSDF(mat, hit, -ray.dir, seed);
            if (bsdf.pdf < EPS && !bsdf.isDelta) {
                break;
            }
            if (mat.type == mat_Volume) {
                currentMedium = hit.frontFace ? bsdf.medium : BSDFMediumInfo(false, false, vec3(1.0), 0.0, 0.0);
            }
            if (ubo.importanceSampling == 1 && !bsdf.isDelta) {
                lightSample = sampleLight(hit, seed);
                BSDFEval eval = evalBSDF(mat, hit, -ray.dir, lightSample.wi);
                if (lightSample.pdf > EPS) {
                    float cosTheta = max(dot(hit.normal, lightSample.wi), 0.0);
                    float wMIS = (lightSample.pdf*lightSample.pdf) / (lightSample.pdf*lightSample.pdf + eval.pdf*eval.pdf);
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
            prevBsdf = bsdf;
            prevHit = hit;

            ray = Ray(hit.p + bsdf.wi * EPS, bsdf.wi);
            hit = intersection(ray, false, INFINITY, stats);

            // Might not be the best place, but we can use the new `hit` here
            if (currentMedium.isVolume) {
                float sigma_t = currentMedium.density;
                float t_scatter = -log(rand(seed)) / sigma_t;
                if (t_scatter < hit.t) {
                    ray.origin += ray.dir * t_scatter;

                    if (ubo.importanceSampling == 1) {
                        Hit scatterHit = Hit(ray.origin, vec3(0.0), t_scatter, true, OBJECT_NONE);
                        LightSample volLight = sampleLight(scatterHit, seed);
                        if (volLight.pdf > EPS) {
                            float phase = phaseFunctionHG(currentMedium.anisotropic, volLight.wi, ray.dir);
                            float wMIS = (volLight.pdf*volLight.pdf) / (volLight.pdf*volLight.pdf + phase*phase);
                            radiance += throughput * currentMedium.absorption * phase * volLight.Le * wMIS / volLight.pdf;
                        }
                    }

                    vec3 incomingDir = ray.dir;
                    ray.dir = sampleHG(currentMedium.anisotropic, ray.dir, seed);
                    throughput *= currentMedium.absorption;

                    prevBsdf.pdf = phaseFunctionHG(currentMedium.anisotropic, ray.dir, incomingDir);
                    prevBsdf.isDelta = false;
                    prevHit.p = ray.origin;

                    hit = intersection(ray, false, INFINITY, stats);
                    continue;
                }
            }
            if (prevBsdf.medium.isDielectric) {
                throughput *= pow(max(prevBsdf.medium.absorption, vec3(1e-4)), vec3(hit.t * prevBsdf.medium.density));
            }
            
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

vec3 computeFragmentColor(in Camera camera, in vec2 fragPos, inout uint seed, float sampleProb, inout PixelInfo pixelInfo, out float takenSamples) {
    vec3 colorSum = vec3(0);
    takenSamples = 0.0;
    for (int i = 0; i < ubo.samplesPerPixel; i++) {
        if (sampleProb >= 1.0 || rand(seed) <= sampleProb) {
            pixelInfo.count += 1;
            vec2 offset = ubo.resolution * vec2(rand(seed), rand(seed)) / ubo.screenSize;
            Ray ray = getRay(camera, fragPos + offset, true, seed);
            vec3 rayColor = traceRay(camera, ray, seed, pixelInfo);
            if (isnan(rayColor.r) || isnan(rayColor.g) || isnan(rayColor.b) || isinf(rayColor.r) || isinf(rayColor.g) || isinf(rayColor.b)) {
                pixelInfo.count -= 1;
                continue;
            }

            if (ubo.clipAccumulation == 1) rayColor = min(rayColor, vec3(50.0));
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
        Statistics selectionStats = Statistics(0, 0);
        selectedHit = rayObjectIntersection(primaryRay, objectBuffer.objects[objectBuffer.selectedObjectId], false, INFINITY, selectionStats);
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
