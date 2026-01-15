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
    float tFinal = INFINITY;
    Object obj = OBJECT_NONE;
    vec3 meshNormal = vec3(0.0, 1.0, 0.0);
    bool meshNormalValid = false;

    for (int i = 0; i < objectBuffer.objectCount; i++) {
        float t = -1.0;
        if (objectBuffer.objects[i].type == obj_Mesh) {
            vec3 hitNormal;
            t = rayMeshIntersectionNormal(ray, meshBuffer.meshes[objectBuffer.objects[i].id], hitNormal);
            if (t >= EPS && t < tFinal) {
                tFinal = t;
                obj = objectBuffer.objects[i];
                meshNormal = hitNormal;
                meshNormalValid = true;
            }
        } else {
            t = rayObjectIntersection(ray, objectBuffer.objects[i]);
            if (t >= EPS && t < tFinal) {
                tFinal = t;
                obj = objectBuffer.objects[i];
                meshNormalValid = false;
            }
        }
    }

    if (obj.type == obj_None) {
        return Hit(vec3(0), vec3(0), INFINITY, true, OBJECT_NONE);
    }
    
    vec3 p = ray.origin + ray.dir * tFinal;
    vec3 normal = meshNormalValid ? meshNormal : getNormal(obj, p);

    bool front_face = true;
    if (dot(ray.dir, normal) > 0.0) {
        normal = -normal;
        front_face = false;
    }

    return Hit(p, normal, tFinal, front_face, obj);
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
    Hit hit = intersection(ray);
    vec3 throughput = vec3(1.0);
    vec3 radiance = vec3(0.0);

    int i = 0;
    ScatterResult result;
    Material mat;
    for (; i < ubo.maxBounces; i++) {
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

    // radiance = vec3(i / float(ubo.maxBounces));
    return radiance;
}

vec3 computeFragmentColor(in Camera camera, inout uint seed) {
    vec3 color = vec3(0);
    for (int i = 0; i < ubo.samplesPerPixel; i++) {
        uint sampleState = pcg_hash(seed + uint(i));
        vec2 offset = vec2(rand(sampleState), rand(sampleState)) / ubo.screenSize;
        Ray ray = getRay(camera, fragPos + offset, true, sampleState);
        vec3 rayColor = traceRay(camera, ray, sampleState);
        color.rgb += rayColor.rgb;
    }
    color.rgb /= float(ubo.samplesPerPixel);

    return color;
}

void main() {
    vec2 uv = fragPos * 0.5 + 0.5;

    vec2 texSize = vec2(textureSize(prevTex, 0));
    vec2 screenCoord = uv * texSize;
    ivec2 pixelCoord = ivec2(screenCoord);

    vec3 prevColor = texelFetch(prevTex, pixelCoord, 0).rgb;

    Camera camera = Camera(ubo.cameraPos, ubo.cameraDir, vec3(0, 1, 0));
    uint seed = initSeed(uvec2(pixelCoord), uint(ubo.frameCount));

    vec3 currColor = vec3(0);
    if (ubo.frameCount <= 1) {
        prevColor = vec3(0.0);

        ivec2 blockCoord = ivec2(round(screenCoord / ubo.lowResolutionScale) * ubo.lowResolutionScale);
        if (ubo.lowResolutionScale == 1.0f || pixelCoord == blockCoord) {
            currColor = computeFragmentColor(camera, seed);
        }
    } else {
        currColor = computeFragmentColor(camera, seed);
    }

    float intersection = 0;
    if (objectBuffer.selectedObjectId >= 0) {
        float t = rayObjectIntersection(getRay(camera, fragPos, false, seed), objectBuffer.objects[objectBuffer.selectedObjectId]);
        if (t > 0.0) intersection = 1;
    }

    float frame = float(max(ubo.frameCount, 1));
    vec3 mixedColor = mix(prevColor, currColor, 1.0 / frame);
    outColor = vec4(mixedColor, intersection);
}
