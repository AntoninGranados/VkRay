import "scene.slang";
import "materials/materials.slang";
import "objects.slang";
import "lights.slang";
import "global.slang";
import "motion.slang";
import "random/utils.slang";
import "colors/utils.slang";
import "environment.slang";
import "adaptive_sampling.slang";
import "camera/camera.slang";

struct ScreenUBO {
    float2 size;
    float aspect;
};

struct RenderUBO {
    int skySlot;
    int skyParamsBase;
    int maxBounces;
    int importanceSampling;
    int clipAccumulation;
    float clipThreshold;
    int varianceSampling;
    int varianceWarmupSamples;
    int cameraMediumCount;
    int4 cameraMedia[MEDIUM_STACK_SIZE / 4];
};

struct UBO {
    int       sampleCount;
    CameraUBO camera;
    ScreenUBO screen;
    RenderUBO render;
};

ConstantBuffer<UBO> ubo;
Sampler2D prevTex;
RWStructuredBuffer<PixelInfo> pixelInfoBuffer;
[[vk::image_format("rgba32f")]] RWTexture2D<float4> outputImage;

static const int MAX_FALSE_INTERFACES = 16;

static const uint BLUE_NOISE_CAMERA_DIMENSIONS = 16u;
static const uint BLUE_NOISE_SEGMENT_DIMENSIONS = 16u;
static const uint BLUE_NOISE_SEGMENTS = 3u;

void collectGBuffer(in Ray ray, inout PixelInfo pixelInfo, inout RngState rng) {
    Statistics dummy = Statistics(0u, 0u);
    Hit firstHit = intersection(ray, false, INFINITY, dummy);

    if (!foundIntersection(firstHit)) {
        pixelInfo.aov.skyMask   = 1u;
        pixelInfo.aov.positionW = float3(0.0);
        pixelInfo.aov.position  = float3(0.0);
        pixelInfo.aov.normalW   = float3(0.0);
        pixelInfo.aov.normal    = float2(0.0);
        pixelInfo.aov.albedo    = float3(0.0);
        pixelInfo.aov.roughness = 0.0;
        return;
    }

    IMaterial material = makeMaterial(getMaterial(firstHit.object));
    BsdfProperties properties = material.evaluate(firstHit, -ray.dir, rng).properties();
    float guideMix = 1.0 / float(pixelInfo.count);

    CameraPose pose = sampleCameraPose(ubo.camera.motionOffset);
    float3 right = pose.right;
    float3 up    = pose.up;

    float3 normalW = normalize(firstHit.normal);
    float3 positionW = firstHit.p;
    float3 albedo = properties.albedo;
    float roughness = properties.roughness;
    float3 hitOff = positionW - pose.eye;
    float3 camDir = pose.dir;
    float2 normal = float2(dot(normalW, right), dot(normalW, up));
    float3 position = float3(dot(hitOff, right), dot(hitOff, up), dot(hitOff, camDir));

    if (pixelInfo.aov.hitValid != 0u) {
        pixelInfo.aov.positionW = lerp(pixelInfo.aov.positionW, positionW, guideMix);
        pixelInfo.aov.position  = lerp(pixelInfo.aov.position,  position,  guideMix);
        pixelInfo.aov.normalW   = normalize(lerp(pixelInfo.aov.normalW, normalW, guideMix));
        pixelInfo.aov.normal    = lerp(pixelInfo.aov.normal,    normal,    guideMix);
        pixelInfo.aov.albedo    = lerp(pixelInfo.aov.albedo,    albedo,    guideMix);
        pixelInfo.aov.roughness = lerp(pixelInfo.aov.roughness, roughness, guideMix);
    } else {
        pixelInfo.aov.positionW = positionW;
        pixelInfo.aov.position  = position;
        pixelInfo.aov.normalW   = normalW;
        pixelInfo.aov.normal    = normal;
        pixelInfo.aov.albedo    = albedo;
        pixelInfo.aov.roughness = roughness;
        pixelInfo.aov.matType   = uint(properties.type);
        pixelInfo.aov.hitValid  = 1u;
    }
}

float3 clampIndirect(float3 contribution, int bounce) {
    if (ubo.render.clipAccumulation == 0 || bounce <= 0) return contribution;
    float sum = contribution.r + contribution.g + contribution.b;
    if (sum <= ubo.render.clipThreshold) return contribution;
    return contribution * (ubo.render.clipThreshold / sum);
}

void initMediumStack(in Ray ray, inout RngState rng) {
    resetMediumStack();
    for (int i = 0; i < ubo.render.cameraMediumCount; i++) {
        Object object = loadObject(ubo.render.cameraMedia[i / 4][i % 4]);
        Hit hit = Hit(ray.origin, float2(0.0), -ray.dir, 0.0, true, object, float3(1.0));
        IMaterial material = makeMaterial(getMaterial(object));
        Optional<MediumEntry> mediumEntry = material.evaluate(hit, -ray.dir, rng).medium(object);
        if (mediumEntry.hasValue) pushMedium(mediumEntry.value);
    }
}

float3 traceRay(in Ray ray, inout RngState rng, inout PixelInfo pixelInfo) {
    Statistics stats = Statistics(0, 0);
    Hit hit = intersection(ray, false, INFINITY, stats);

    pixelInfo.bvhChecks      = stats.bvhChecks;
    pixelInfo.triangleChecks = stats.triangleChecks;

    float3 throughput = float3(1.0);
    float3 radiance = float3(0.0);
    initMediumStack(ray, rng);
    int falseInterfaces = 0;

    int i = 0;
    BSDFSample bsdf;
    LightSample lightSample;

    BSDFSample prevBsdf = BSDFSample(
        float3(0.0),
        float3(0.0),
        0.0,
        false,
        BSDFMediumInfo(false, false, float3(1.0), 1.0, 0.0, 0.0)
    );
    Hit prevHit;
    bool prevIsSkipped = false;

    for (uint segment = 0u; i < ubo.render.maxBounces; i++, segment++) {
        if (segment < BLUE_NOISE_SEGMENTS)
            setRngDimensions(rng, BLUE_NOISE_CAMERA_DIMENSIONS + segment * BLUE_NOISE_SEGMENT_DIMENSIONS, BLUE_NOISE_SEGMENT_DIMENSIONS);
        else
            setRngDimensions(rng, 0u, 0u);

        if (!foundIntersection(hit)) {
            radiance += clampIndirect(throughput * skyColor(ubo.render.skySlot, ubo.render.skyParamsBase, ray.dir), i - 1);
            break;
        }

        BSDFMediumInfo medium = activeMedium();
        if (medium.isVolume) {
            float sigma_t = medium.density;
            float omega   = medium.scatterAlbedo;
            float sigma_s = sigma_t * omega;
            float sigma_a = sigma_t * (1.0 - omega);

            float t_scatter = (sigma_s > EPS) ? (-log(rand(rng)) / sigma_s) : INFINITY;
            if (t_scatter < hit.t) {
                ray.origin += ray.dir * t_scatter;

                throughput *= pow(max(medium.absorption, float3(1e-4)), float3(sigma_a * t_scatter));

                if (ubo.render.importanceSampling == 1) {
                    Hit scatterHit = Hit(ray.origin, float2(0), float3(0.0), t_scatter, true, OBJECT_NONE, float3(1.0));
                    LightSample volLight = sampleLight(scatterHit, rng);
                    prevIsSkipped = volLight.skip;
                    if (volLight.pdf > EPS) {
                        float phase = phaseFunctionHG(medium.anisotropic, volLight.wi, ray.dir);
                        float wMIS = powerHeuristic(volLight.pdf, phase);
                        radiance += clampIndirect(throughput * medium.absorption * phase * volLight.Le * wMIS / volLight.pdf, i);
                    }
                } else {
                    prevIsSkipped = false;
                }

                float3 incomingDir = ray.dir;
                ray.dir = sampleHG(medium.anisotropic, ray.dir, rng);
                throughput *= medium.absorption;

                prevBsdf.pdf = phaseFunctionHG(medium.anisotropic, ray.dir, incomingDir);
                prevBsdf.isDelta = false;
                prevHit.p = ray.origin;

                hit = intersection(ray, false, INFINITY, stats);
                continue;
            } else {
                throughput *= pow(max(medium.absorption, float3(1e-4)), float3(sigma_a * hit.t));
            }
        }

        IMaterial material = makeMaterial(getMaterial(hit.object));
        IBsdf surface = material.evaluate(hit, -ray.dir, rng);
        BsdfProperties properties = surface.properties();

        Optional<MediumEntry> mediumEntry = surface.medium(hit.object);
        if (mediumEntry.hasValue && falseInterfaces < MAX_FALSE_INTERFACES && isFalseInterface(mediumEntry.value.priority, hit)) {
            falseInterfaces++;
            crossInterface(mediumEntry.value, hit);
            ray = Ray(hit.p + ray.dir * EPS, ray.dir);
            hit = intersection(ray, false, INFINITY, stats);
            i--;
            continue;
        }

        if (any(properties.emission > 0.0)) {
            if (i == 0) {
                radiance = properties.emission;
                break;
            }

            float3 Le = properties.emission;
            float w = 1.0;
            if (ubo.render.importanceSampling == 1 && !prevBsdf.isDelta && !prevIsSkipped) {
                float pdfL = lightPDF(hit.object.lightId, length(hit.p - prevHit.p), hit.normal, ray.dir);
                w = powerHeuristic(prevBsdf.pdf, pdfL);
            }

            radiance += clampIndirect(throughput * w * Le, i - 1);
            break;
        }

        bsdf = surface.sample(hit, -ray.dir, rng);
        if (bsdf.pdf < EPS && !bsdf.isDelta) {
            break;
        }
        bool isSkipped = false;
        if (ubo.render.importanceSampling == 1 && !bsdf.isDelta) {
            lightSample = sampleLight(hit, rng);
            isSkipped = lightSample.skip;
            if (lightSample.pdf > EPS) {
                BSDFEval eval = surface.eval(hit, -ray.dir, lightSample.wi);
                float cosTheta = max(dot(hit.normal, lightSample.wi), 0.0);
                float wMIS = powerHeuristic(lightSample.pdf, eval.pdf);
                radiance += clampIndirect(throughput * eval.f * cosTheta * lightSample.Le * wMIS / lightSample.pdf, i);
            }
        }

        throughput *= bsdf.weight;

        // Russian Roulette
        if (i >= 2 && !bsdf.isDelta) {
            float p = clamp(luma(throughput), 0.1, 1.0);
            if (rand(rng) > p) break;
            throughput /= p;
        }
        if (!properties.passThrough && !bsdf.medium.isVolume) {
            prevBsdf = bsdf;
            prevHit = hit;
            prevIsSkipped = isSkipped;
        } else {
            prevIsSkipped = true;
        }

        ray = Ray(hit.p + bsdf.wi * EPS, bsdf.wi);
        hit = intersection(ray, false, INFINITY, stats);
    }
    if (i == ubo.render.maxBounces) radiance = float3(0.0);

    pixelInfo.bounces = uint(i);
    return radiance;
}

float3 computeFragmentColor(in float2 fragPos, inout RngState rng, float sampleProb, inout PixelInfo pixelInfo, out float takenSamples) {
    float3 colorSum = float3(0);
    takenSamples = 0.0;
    if (sampleProb >= 1.0 || rand(rng) <= sampleProb) {
        pixelInfo.count++;
        float2 offset = float2(rand(rng), rand(rng)) / ubo.screen.size;
        Ray ray = getRay(ubo.camera, fragPos + offset, rng);
        collectGBuffer(ray, pixelInfo, rng);
        float3 rayColor = traceRay(ray, rng, pixelInfo);
        if (isnan(rayColor.r) || isnan(rayColor.g) || isnan(rayColor.b) || isinf(rayColor.r) || isinf(rayColor.g) || isinf(rayColor.b)) {
            pixelInfo.count--;
        } else {
            colorSum.rgb += rayColor.rgb;

            takenSamples = 1.0;
            updateVariance(luma(rayColor.rgb), pixelInfo);
        }
    }

    return colorSum;
}

[shader("compute")]
[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID) {
    int2 texSize = textureSize(prevTex);
    int2 pixelCoord = int2(dispatchThreadId.xy);

    if (pixelCoord.x >= texSize.x || pixelCoord.y >= texSize.y) return;

    float2 screenCoord = float2(pixelCoord) + float2(0.5);
    float2 uv = screenCoord / float2(texSize);
    float2 fragPos = uv * 2.0 - 1.0;

    float3 prevColor = prevTex.Load(int3(pixelCoord, 0)).rgb;
    if (ubo.sampleCount <= 1) {
        prevColor = float3(0);

        uint varianceIndex = varianceIndexFromCoord(pixelCoord, texSize);
        PixelInfo initInfo = pixelInfoBuffer[varianceIndex];
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
        pixelInfoBuffer[varianceIndex] = initInfo;
    }

    RngState rng = initBlueNoiseRngState(uint2(pixelCoord), uint(ubo.sampleCount));
    setRngDimensions(rng, 0u, BLUE_NOISE_CAMERA_DIMENSIONS);

    uint blockVarianceIndex = varianceIndexFromCoord(pixelCoord, texSize);
    PixelInfo pixelInfo = pixelInfoBuffer[blockVarianceIndex];
    float prevCount = max(float(pixelInfo.count), 0.0);

    float sampleProb = 1.0;
    if (ubo.render.varianceSampling != 0)
        sampleProb = computeSampleProbability(pixelInfo, prevTex, ubo.render.varianceWarmupSamples, pixelCoord, texSize);

    float takenSamples = 0.0;
    float3 colorSum = computeFragmentColor(fragPos, rng, sampleProb, pixelInfo, takenSamples);
    PixelInfo updatedInfo = pixelInfoBuffer[blockVarianceIndex];
    updatedInfo.aov             = pixelInfo.aov;
    updatedInfo.mean            = pixelInfo.mean;
    updatedInfo.m2              = pixelInfo.m2;
    updatedInfo.count           = pixelInfo.count;
    updatedInfo.bounces         = pixelInfo.bounces;
    updatedInfo.bvhChecks       = pixelInfo.bvhChecks;
    updatedInfo.triangleChecks  = pixelInfo.triangleChecks;
    updatedInfo.varianceProba   = pixelInfo.varianceProba;
    pixelInfoBuffer[blockVarianceIndex] = updatedInfo;

    float totalCount = prevCount + takenSamples;
    float3 mixedColor = prevColor;
    if (totalCount > 0.0) {
        mixedColor = (prevColor * prevCount + colorSum) / totalCount;
    }
    outputImage[pixelCoord] = float4(mixedColor, 1.0);
}
