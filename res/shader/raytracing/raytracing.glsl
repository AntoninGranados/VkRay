#version 450

#include "utils.glsl"
#include "materials.glsl"
#include "objects.glsl"
#include "random.glsl"

// ================== INPUTS/OUTPUTS ==================
layout(location = 0) in vec2 fragPos;
layout(location = 0) out vec4 outColor;

layout(std140, set = 0, binding = 0) uniform UBO {
    vec3 cameraPos;
    vec3 cameraDir;

    vec2 screenSize;
    float aspect;
    float tanHFov;
    
    int frameCount;
    float time;

    int timeOfDay;
} ubo;
layout(set = 0, binding = 1) buffer readonly SSBO {
    int sphereCount;
    int selectedSphereId;
    Sphere spheres[];
} ssbo;
layout(set = 0, binding = 2) uniform sampler2D prevTex;

#define PLANE_COUNT 10
uint planeCount = 0;
Plane planes[PLANE_COUNT];

bool pushPlane(in Plane plane) {
    if (planeCount >= PLANE_COUNT) return false;

    planes[planeCount] = plane;
    planeCount++;
    return true;
}

Material getMaterial(in Hit hit) {
    if (hit.type == obj_Sphere)
        return ssbo.spheres[hit.idx].mat;
    else if (hit.type == obj_Plane)
        return planes[hit.idx].mat;
    return DEFAULT_MATERIAL;
}

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
    float t = INFINITY;
    int idx = -1;
    Enum type = obj_None;

    for (int i = 0; i < ssbo.sphereCount; i++) {
        float new_t = raySphereIntersection(ray, ssbo.spheres[i]);
        if (new_t >= EPS && new_t < t) {
            t = new_t;
            idx = i;
            type = obj_Sphere;
        }
    }
    
    for (int i = 0; i < planeCount; i++) {
        float new_t = rayPlaneIntersection(ray, planes[i]);
        if (new_t >= EPS && new_t < t) {
            t = new_t;
            idx = i;
            type = obj_Plane;
        }
    }

    vec3 p = ray.origin + ray.dir * t;

    vec3 normal;
    if (type == obj_Sphere)
        normal = normalize(p - ssbo.spheres[idx].center);
    else if (type == obj_Plane)
        normal = planes[idx].normal;

    bool front_face = true;
    if (dot(ray.dir, normal) > 0.0) {
        normal = -normal;
        front_face = false;
    }

    return Hit(p, normal, t, front_face, idx, type);
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
    float factor = 1;

    int i = 0;
    for (; i < MAX_BOUNCE_DEPTH; i++) {
        if (hit.type != obj_None) {
            vec3 attenuation;
            Ray scattered;

            bool isScattered = scatter(getMaterial(hit), ray, hit, attenuation, scattered, seed);
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

    pushPlane(Plane(vec3(0, -1, 0), vec3(0, 1, 0), ANIMATED_MATERIAL));

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

    float idFloat = 0.0;
    if (ssbo.selectedSphereId >= 0) {
        float t = raySphereIntersection(getRay(camera, fragPos), ssbo.spheres[ssbo.selectedSphereId]);
        if (t > 0.0)
            idFloat = 1.0;

    }

    float frame = float(max(ubo.frameCount, 1));
    vec3 mixedColor = mix(prevColor, currColor, 1.0 / frame);
    outColor = vec4(mixedColor, idFloat);
}
