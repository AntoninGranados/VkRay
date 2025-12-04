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

    if (ubo.timeOfDay == 0) {           // Day
        zenith = vec3(0.5, 0.7, 1.0);
        horizon = vec3(1.0, 1.0, 1.0);
    } else if (ubo.timeOfDay == 1) {    // Sunset
        zenith = vec3(0.2, 0.1, 0.4);
        horizon = vec3(1.0, 0.4, 0.2);
    } else {                            // Night
        zenith  = vec3(0.01, 0.01, 0.03);
        horizon = vec3(0.05, 0.05, 0.1);
    }
    
    vec3 color = mix(horizon, zenith, t);
    color += vec3(0.05, 0.02, 0.0) * pow(1.0 - t, 3.0);
    return color;
}

vec3 traceRay(in Camera camera, in Ray ray, inout vec3 seed) {
    Hit hit = intersection(ray);
    vec3 color = vec3(1);

    int i = 0;
    for (; i < MAX_BOUNCE_DEPTH; i++) {
        if (foundIntersection(hit)) {
            vec3 attenuation;
            Ray scattered;

            bool isScattered = scatter(
                getMaterial(hit.object),
                ray,
                hit,
                attenuation,
                scattered,
                seed
            );
            color *= attenuation;
            if (!isScattered) {
                break;
            }

            hit = intersection(scattered);
            ray = scattered;
        } else {
            color *= skyColor(ray.dir);
            break;
        }
    }
    if (i == MAX_BOUNCE_DEPTH)
        color = vec3(0.0);

    color = min(max(color, vec3(0)), vec3(1));
    return color;
}

void main() {
    vec3 seed = initSeed(fragPos, ubo.time);

    Camera camera = Camera(ubo.cameraPos, ubo.cameraDir, vec3(0, 1, 0));

    // pushPlane(Plane(vec3(0, -1, 0), vec3(0, 1, 0), ANIMATED_MATERIAL));

    vec2 uv = fragPos * 0.5 + 0.5;
    vec3 prevColor = texture(prevTex, uv).rgb;
    if (ubo.frameCount <= 1) prevColor = vec3(0.0);

    vec3 currColor = vec3(0);
    for (int i = 0; i < SAMPLES_PER_PIXEL; i++) {
        vec2 offset = vec2(rand(seed), rand(seed)) / ubo.screenSize;
        Ray ray = getRay(camera, fragPos + offset);
        vec3 color = traceRay(camera, ray, seed);
        currColor.rgb += color.rgb;
    }
    currColor.rgb /= SAMPLES_PER_PIXEL;

    float intersection = 0;
    if (objectBuffer.selectedObjectId >= 0) {
        float t = rayObjectIntersection(getRay(camera, fragPos), objectBuffer.objects[objectBuffer.selectedObjectId]);
        if (t > 0.0)
            intersection = 1;
    }

    float frame = float(max(ubo.frameCount, 1));
    vec3 mixedColor = mix(prevColor, currColor, 1.0 / frame);
    outColor = vec4(mixedColor, intersection);
}
