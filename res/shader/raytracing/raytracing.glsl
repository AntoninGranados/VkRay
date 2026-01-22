#version 450

#include "inputs.glsl"
#include "utils.glsl"
#include "materials.glsl"
#include "objects.glsl"
#include "lights.glsl"
#include "global.glsl"
#include "random.glsl"


Ray getRay(Camera camera, vec2 ndc_pos, in bool enableFocus, inout uint seed) {
    vec3 forward = normalize(camera.dir);
    vec3 right   = normalize(cross(forward, camera.up));
    vec3 up      = cross(right, forward);

    // ndc_pos is in [-1, 1]; convert to [0, 1] UV space
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
    Hit bestHit = Hit(vec3(0), vec3(0), INFINITY, true, OBJECT_NONE);

    for (int i = 0; i < objectBuffer.objectCount; i++) {
        Hit hit = rayObjectIntersection(ray, objectBuffer.objects[i]);
        if (foundIntersection(hit) && hit.t < bestHit.t) {
            bestHit = hit;
        }
    }

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
    
    vec3 color = mix(horizon, zenith, t);
    color += vec3(0.05, 0.02, 0.0) * pow(1.0 - t, 3.0);
    return color;
}

vec3 traceRay(in Camera camera, in Ray ray, inout uint seed) {
    Ray primaryRay = ray;
    Hit hit = intersection(ray);
    vec3 throughput = vec3(1.0);
    vec3 radiance = vec3(0.0);

    int i = 0;
    ScatterResult result;
    Material mat;
    for (; i < ubo.maxBounces; i++) {
        if (ubo.debugView == debug_Normal) break;
        
        if (foundIntersection(hit)) {
            mat = getMaterial(hit.object);

            if (mat.type == mat_Emissive) {
                radiance += throughput * mat.albedo * emissiveIntensity(mat);
                break;
            }

            scatter(
                mat,
                ray,
                hit,
                result,
                seed
            );
            throughput *= result.attenuation;
            if (!result.isScattered) break;

            vec3 direct = importanceSampleLight(mat, hit, result, seed);
            radiance += throughput * direct;

            ray = result.scattered;
            hit = intersection(ray);
        } else {
            radiance += throughput * skyColor(ray.dir);
            break;
        }
    }
    if (i == ubo.maxBounces)
        radiance = vec3(0.0);

    // Debug visualisations
    if (ubo.debugView == debug_Bounces) {
        return vec3(i / float(ubo.maxBounces));
    }
    if (ubo.debugView == debug_Normal) {
        return foundIntersection(hit) ? (hit.normal * 0.5 + 0.5) : vec3(0.0);
    }
    return radiance;
}

float luma(vec3 c) {
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
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
    pixelInfo.count += 1.0;
    float delta = value - pixelInfo.mean;
    pixelInfo.mean += delta / pixelInfo.count;
    float delta2 = value - pixelInfo.mean;
    pixelInfo.m2 += delta * delta2;
}

float computeTemporalSigma(in PixelInfo pixelInfo) {
    float variance = (pixelInfo.count > 1.0) ? (pixelInfo.m2 / (pixelInfo.count - 1.0)) : 0.0;
    float varianceMean = (pixelInfo.count > 0.0) ? (variance / pixelInfo.count) : 0.0;
    return sqrt(max(varianceMean, 0.0));
}

float computeSpatialVariance(ivec2 blockCoord, vec2 texSize) {
    const int radius = 1;
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
    float temporalSigma = computeTemporalSigma(pixelInfo);
    float spatialSigma = computeSpatialVariance(blockCoord, texSize);

    float sigma = mix(temporalSigma, spatialSigma, 0.5);
    float minAdaptiveSamples = max(float(ubo.varianceWarmupSamples), 0.0);
    float proba = clamp(sigma * 8.0, 0.01, 1.0);
    proba = sqrt(proba);
    pixelInfo.varianceProba = proba;
    return (pixelInfo.count < minAdaptiveSamples) ? 1.0 : proba;
}

vec3 computeFragmentColor(in Camera camera, in vec2 fragPos, inout uint seed, float sampleProb, inout PixelInfo pixelInfo, out float takenSamples) {
    vec3 colorSum = vec3(0);
    takenSamples = 0.0;
    for (int i = 0; i < ubo.samplesPerPixel; i++) {
        if (sampleProb >= 1.0 || rand(seed) <= sampleProb) {
            vec2 offset = vec2(rand(seed), rand(seed)) / ubo.screenSize;
            Ray ray = getRay(camera, fragPos + offset, true, seed);
            vec3 rayColor = traceRay(camera, ray, seed);
            colorSum.rgb += rayColor.rgb;

            takenSamples += 1.0;
            updateVariance(luma(rayColor.rgb), pixelInfo);
        }
    }
    return colorSum;
}

void main() {
    vec2 uv = fragPos * 0.5 + 0.5;

    ivec2 texSize = textureSize(prevTex, 0);
    vec2 screenCoord = uv * vec2(texSize);
    ivec2 pixelCoord = ivec2(screenCoord);

    float prevScale = max(ubo.prevResolution, 1.0);
    ivec2 prevBlockCoord = pixelCoord;
    if (prevScale > 1.0) {
        prevBlockCoord = ivec2(floor(screenCoord / prevScale) * prevScale);
        prevBlockCoord = clamp(prevBlockCoord, ivec2(0), texSize - ivec2(1));
    }

    // Init values in render start
    vec3 prevColor = texelFetch(prevTex, prevBlockCoord, 0).rgb;
    if (ubo.frameCount <= 1) {
        prevColor = vec3(0);
        
        uint varianceIndex = varianceIndexFromCoord(pixelCoord, texSize);
        pixelInfoBuffer.pixels[varianceIndex] = PixelInfo(0.0, 0.0, 0.0, 0.0);
    }

    Camera camera = Camera(ubo.cameraPos, ubo.cameraDir, vec3(0, 1, 0));
    uint seed = initSeed(uvec2(pixelCoord), uint(ubo.frameCount));

    
    ivec2 blockCoord = blockCoordFromResolution(pixelCoord, screenCoord, texSize, ubo.resolution);
    uint blockVarianceIndex = varianceIndexFromCoord(blockCoord, texSize);
    PixelInfo pixelInfo = pixelInfoBuffer.pixels[blockVarianceIndex];
    float prevCount = max(pixelInfo.count, 0.0);
    
    // Compute sample probability
    float sampleProb = 1.0;
    if (ubo.varianceSampling != 0)
        sampleProb = computeSampleProbability(pixelInfo, blockCoord, texSize, ubo.prevResolution);

    // Compute sample color
    float takenSamples = 0.0;
    vec3 colorSum = vec3(0);
    if (ubo.resolution == 1.0f || pixelCoord == blockCoord) {
        colorSum = computeFragmentColor(camera, fragPos, seed, sampleProb, pixelInfo, takenSamples);
        pixelInfoBuffer.pixels[blockVarianceIndex] = PixelInfo(pixelInfo.mean, pixelInfo.m2, pixelInfo.count, pixelInfo.varianceProba);
    }
    
    if (ubo.resolution < ubo.prevResolution) prevColor = colorSum / max(takenSamples, 1.0);

    float intersection = 0;
    if (objectBuffer.selectedObjectId >= 0) {
        Hit hit = rayObjectIntersection(getRay(camera, fragPos, false, seed), objectBuffer.objects[objectBuffer.selectedObjectId]);
        if (foundIntersection(hit)) intersection = 1;
    }

    float totalCount = prevCount + takenSamples;
    vec3 mixedColor = prevColor;
    if (totalCount > 0.0) {
        mixedColor = (prevColor * prevCount + colorSum) / totalCount;
    }
    outColor = vec4(mixedColor, intersection);
}
