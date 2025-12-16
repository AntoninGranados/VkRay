#version 450

#include "inputs.glsl"
#include "utils.glsl"
#include "materials.glsl"
#include "objects.glsl"
#include "global.glsl"
#include "random.glsl"


Ray getRay(Camera camera, vec2 ndc_pos) {
    vec3 forward = normalize(camera.dir);
    vec3 right   = normalize(cross(forward, camera.up));
    vec3 up      = cross(right, forward);

    // ndc_pos is in [-1, 1]; convert to [0, 1] UV space
    float scr_x = ndc_pos.x * 0.5f + 0.5f;
    float scr_y = ndc_pos.y * 0.5f + 0.5f;
    
    float cam_x = (2.f * scr_x - 1.f) * ubo.aspect * ubo.tanHFov;
    float cam_y = (1.f - 2.f * scr_y) * ubo.tanHFov;

    vec3 dir = cam_x * right + cam_y * up + forward;
    dir = normalize(dir);

    return Ray(camera.pos, dir);
}

Hit intersection(in Ray ray) {
    float tFinal = INFINITY;
    Object obj = OBJECT_NONE;

    for (int i = 0; i < objectBuffer.objectCount; i++) {
        float t = rayObjectIntersection(ray, objectBuffer.objects[i]);
        if (t >= EPS && t < tFinal) {
            tFinal = t;
            obj = objectBuffer.objects[i];
        }
    }

    if (obj.type == obj_None) {
        return Hit(vec3(0), vec3(0), INFINITY, true, OBJECT_NONE);
    }
    
    vec3 p = ray.origin + ray.dir * tFinal;
    vec3 normal = getNormal(obj, p);

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

// Uniform sample on a box surface; returns a point, sets the outward normal and pdf over area.
vec3 sampleBoxSurface(Box box, inout vec3 seed, out vec3 normal, out float pdfA) {
    vec3 size = box.cornerMax - box.cornerMin;
    float areaXY = size.x * size.y;
    float areaYZ = size.y * size.z;
    float areaZX = size.z * size.x;
    float totalArea = 2.0 * (areaXY + areaYZ + areaZX);
    pdfA = totalArea > EPS ? 1.0 / totalArea : 0.0;

    float r = rand(seed) * totalArea;

    // z-min face
    if (r < areaXY) {
        float u = rand(seed);
        float v = rand(seed);
        normal = vec3(0.0, 0.0, -1.0);
        return vec3(
            mix(box.cornerMin.x, box.cornerMax.x, u),
            mix(box.cornerMin.y, box.cornerMax.y, v),
            box.cornerMin.z
        );
    }
    r -= areaXY;

    // z-max face
    if (r < areaXY) {
        float u = rand(seed);
        float v = rand(seed);
        normal = vec3(0.0, 0.0, 1.0);
        return vec3(
            mix(box.cornerMin.x, box.cornerMax.x, u),
            mix(box.cornerMin.y, box.cornerMax.y, v),
            box.cornerMax.z
        );
    }
    r -= areaXY;

    // x-min face
    if (r < areaYZ) {
        float u = rand(seed);
        float v = rand(seed);
        normal = vec3(-1.0, 0.0, 0.0);
        return vec3(
            box.cornerMin.x,
            mix(box.cornerMin.y, box.cornerMax.y, u),
            mix(box.cornerMin.z, box.cornerMax.z, v)
        );
    }
    r -= areaYZ;

    // x-max face
    if (r < areaYZ) {
        float u = rand(seed);
        float v = rand(seed);
        normal = vec3(1.0, 0.0, 0.0);
        return vec3(
            box.cornerMax.x,
            mix(box.cornerMin.y, box.cornerMax.y, u),
            mix(box.cornerMin.z, box.cornerMax.z, v)
        );
    }
    r -= areaYZ;

    // y-min face
    if (r < areaZX) {
        float u = rand(seed);
        float v = rand(seed);
        normal = vec3(0.0, -1.0, 0.0);
        return vec3(
            mix(box.cornerMin.x, box.cornerMax.x, u),
            box.cornerMin.y,
            mix(box.cornerMin.z, box.cornerMax.z, v)
        );
    }

    // y-max face (fallback)
    float u = rand(seed);
    float v = rand(seed);
    normal = vec3(0.0, 1.0, 0.0);
    return vec3(
        mix(box.cornerMin.x, box.cornerMax.x, u),
        box.cornerMax.y,
        mix(box.cornerMin.z, box.cornerMax.z, v)
    );
}

vec3 traceRay(in Camera camera, in Ray ray, inout vec3 seed) {
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

            if (ubo.importanceSampling == 1 && result.isDiffuse) {
                int lightIdx = getLightId();
                if (lightIdx >= 0) {
                    Object lightObj = objectBuffer.objects[lightIdx];
                    uint lightId = lightObj.id;

                    // New: box importance sampling (hardcoded for now)
                    if (lightObj.type == obj_Box) {
                        Box lightBox = boxBuffer.boxes[lightId];
                        vec3 lightNormal;
                        float pdfA;
                        vec3 lightPoint = sampleBoxSurface(lightBox, seed, lightNormal, pdfA);

                        vec3 toLight = lightPoint - result.scattered.origin;
                        float dist2 = dot(toLight, toLight);
                        float dist = sqrt(dist2);
                        vec3 toLightDir = toLight / dist;

                        float cosSurface = max(dot(hit.normal, toLightDir), 0.0);
                        float cosLight = max(dot(-toLightDir, lightNormal), 0.0);

                        if (cosSurface > 0.0 && cosLight > 0.0 && pdfA > 0.0) {
                            Ray shadowRay = Ray(result.scattered.origin, toLightDir);
                            Hit shadowHit = intersection(shadowRay);
                            bool visible = foundIntersection(shadowHit) && shadowHit.t >= dist - EPS;

                            if (visible) {
                                float pdfW = pdfA * dist2 / max(cosLight, EPS);

                                Material lightMat = getMaterial(lightObj);
                                vec3 Le = lightMat.albedo * emissiveIntensity(lightMat);
                                vec3 direct = (mat.albedo / 3.14159265) * Le * cosSurface / max(pdfW, EPS);

                                radiance += throughput * direct;
                            }
                        }
                    }
                    // Fallback: sphere importance sampling (kept functional while the previous block shows the legacy version)
                    else if (lightObj.type == obj_Sphere) {
                        Sphere lightSphere = sphereBuffer.spheres[lightId];

                        // Uniform sample on sphere surface
                        vec3 onLightDir = normalize(randomInSphere(seed));
                        vec3 lightPoint = lightSphere.center + onLightDir * lightSphere.radius;

                        vec3 toLight = lightPoint - result.scattered.origin;
                        float dist2 = dot(toLight, toLight);
                        float dist = sqrt(dist2);
                        vec3 toLightDir = toLight / dist;

                        float cosSurface = max(dot(hit.normal, toLightDir), 0.0);
                        vec3 lightNormal = (lightPoint - lightSphere.center) / lightSphere.radius;
                        float cosLight = max(dot(-toLightDir, lightNormal), 0.0);

                        if (cosSurface > 0.0 && cosLight > 0.0) {
                            Ray shadowRay = Ray(result.scattered.origin, toLightDir);
                            Hit shadowHit = intersection(shadowRay);
                            bool visible = foundIntersection(shadowHit) && shadowHit.t >= dist - EPS;

                            if (visible) {
                                float lightArea = 4.0 * 3.14159265 * lightSphere.radius * lightSphere.radius;
                                float pdfA = 1.0 / lightArea;
                                float pdfW = pdfA * dist2 / max(cosLight, EPS);

                                Material lightMat = getMaterial(lightObj);
                                vec3 Le = lightMat.albedo * emissiveIntensity(lightMat);
                                vec3 direct = (mat.albedo / 3.14159265) * Le * cosSurface / max(pdfW, EPS);

                                radiance += throughput * direct;
                            }
                        }
                    }
                }
            }

            // Continue path
            ray = result.scattered;
            hit = intersection(ray);
        } else {
            radiance += throughput * skyColor(ray.dir);
            break;
        }
    }
    if (i == ubo.maxBounces)
        radiance = vec3(0.0);

    return radiance;
}

vec3 computeFragmentColor(in Camera camera, inout vec3 seed) {
    vec3 color = vec3(0);
    for (int i = 0; i < ubo.samplesPerPixel; i++) {
        vec2 offset = vec2(rand(seed), rand(seed)) / ubo.screenSize;
        Ray ray = getRay(camera, fragPos + offset);
        vec3 rayColor = traceRay(camera, ray, seed);
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
    vec3 seed = initSeed(fragPos, ubo.time);

    vec3 currColor = vec3(0);
    if (ubo.frameCount <= 1) {
        prevColor = vec3(0.0);

        ivec2 blockCoord = ivec2(round(screenCoord / 10.0) * 10.0);
        if (pixelCoord == blockCoord) {
            currColor = computeFragmentColor(camera, seed);
        }
    } else {
        currColor = computeFragmentColor(camera, seed);
    }

    float intersection = 0;
    if (objectBuffer.selectedObjectId >= 0) {
        float t = rayObjectIntersection(getRay(camera, fragPos), objectBuffer.objects[objectBuffer.selectedObjectId]);
        if (t > 0.0) intersection = 1;
    }

    float frame = float(max(ubo.frameCount, 1));
    vec3 mixedColor = mix(prevColor, currColor, 1.0 / frame);
    outColor = vec4(mixedColor, intersection);
}
