#version 450

// ================== STRUCTS ==================
struct Camera {
    vec3 pos;
    vec3 dir;
    vec3 up;
};

struct Ray {
    vec3 origin;
    vec3 dir;
};

#define Enum uint

#define mat_Lambertian  Enum(0x0000)
#define mat_Metal       Enum(0x0001)
#define mat_Dielectric  Enum(0x0002)
#define mat_Emissive    Enum(0x0003)
#define mat_Animated    Enum(0x0004)
struct Material {
    Enum type;
    vec3 albedo;
    float fuzz;
    float refraction_index;
    float intensity;
};

struct Sphere {
    vec3 center;
    float radius;
    Material mat;
};

struct Plane {
    vec3 point;
    vec3 normal;
    Material mat;
};

#define obj_None    0x0000
#define obj_Sphere  0x0001
#define obj_Plane   0x0002
struct Hit {
    vec3 p;
    vec3 normal;
    float t;
    bool front_face;
    int idx;
    Enum type;
};

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
    Sphere spheres[];
} ssbo;
layout(set = 0, binding = 2) uniform sampler2D prevTex;

// ================== UTILS ==================
#define MAX_BOUNCE_DEPTH 10 // TODO: make it a uniform
#define SAMPLES_PER_PIXEL 2 // TODO: make it a uniform
#define EPS 1e-3

#define DEFAULT_MATERIAL                     Material(mat_Lambertian, vec3(1,0,1)*0.7, 0.0, 0.0, 0.0)
#define LAMBERTIAN_MATERIAL(albedo)          Material(mat_Lambertian, albedo, 0.0, 0.0, 0.0)
#define METAL_MATERIAL(albedo, fuzz)         Material(mat_Metal, albedo, fuzz, 0.0, 0.0)
#define DIELECTRIX_MATERIAL(albedo, index)   Material(mat_Dielectric, albedo, 0.0, index, 0.0)
#define EMISSIVE_MATERIAL(albedo, intensity) Material(mat_Emissive, albedo, 0.0, 0.0, intensity)
#define ANIMATED_MATERIAL                    Material(mat_Animated, vec3(0), 0.0, 0.0, 0.0)


#define PLANE_COUNT 10
uint planeCount = 0;
Plane planes[PLANE_COUNT];

bool pushPlane(in Plane plane) {
    if (planeCount >= PLANE_COUNT) return false;

    planes[planeCount] = plane;
    planeCount++;
    return true;
}

// ================== RANDOM ==================
vec3 initSeed() {
    return vec3(
        fract(sin(dot(fragPos , vec2(12.9898,78.233))) * 43758.5453 + ubo.time),
        fract(sin(dot(fragPos , vec2(93.9898,67.345))) * 43758.5453 + ubo.time),
        fract(sin(dot(fragPos , vec2(56.1234,12.345))) * 43758.5453 + ubo.time)
    );
}

float hash13(vec3 p3) {
    p3  = fract(p3 * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float rand(inout vec3 seed) {
    float r = hash13(seed);
    seed += vec3(1.0, 1.0, 1.0);
    return r;
}

vec3 randomInSphere(inout vec3 seed) {
    while (true) {
        vec3 p = vec3(rand(seed), rand(seed), rand(seed)) * 2 - 1;
        float lensq = dot(p, p);
        if (1e-160 < lensq && lensq <= 1)
            return p / sqrt(lensq);
    }
}

vec3 randomInHemisphere(inout vec3 seed, vec3 normal) {
    vec3 rand = randomInSphere(seed);
    if (dot(rand, normal) < 0.0f)
        return -rand;
    return rand;
}

// ================== MATERIAL ==================
bool scatterLambertian(in Material mat, in Ray ray, in Hit hit, out vec3 attenuation, out Ray scattered, inout vec3 seed) {
    vec3 dir = hit.normal + randomInSphere(seed);
    if (length(dir) < EPS) dir = hit.normal;
    dir = normalize(dir);

    scattered = Ray(hit.p + hit.normal * EPS, dir);
    attenuation = mat.albedo / 3.14159265;

    return true;
}

bool scatterMetal(in Material mat, in Ray ray, in Hit hit, out vec3 attenuation, out Ray scattered, inout vec3 seed) {
    vec3 dir = reflect(ray.dir, hit.normal);
    dir = normalize(dir);
    dir = normalize(dir + randomInSphere(seed) * mat.fuzz);

    scattered = Ray(hit.p + hit.normal * EPS, dir);
    attenuation = mat.albedo;

    return true;
}

float schlick_approx(float cosine, float ri) {
    float r0 = (1 - ri) / (1 + ri);
    r0 = r0*r0;
    return r0 + (1-r0) * pow((1 - cosine),5);
}

bool scatterDielectric(in Material mat, in Ray ray, in Hit hit, out vec3 attenuation, out Ray scattered, inout vec3 seed) {
    float ri = hit.front_face ? (1.0/mat.refraction_index) : mat.refraction_index;

    float cos_theta = min(dot(-ray.dir, hit.normal), 1.0);
    float sin_theta = sqrt(1.0 - cos_theta*cos_theta);
    
    vec3 dir;
    if (ri * sin_theta > 1 || schlick_approx(cos_theta, ri) > rand(seed))
        dir = reflect(ray.dir, hit.normal);
    else
        dir = refract(ray.dir, hit.normal, ri);

    float side = dot(hit.normal, dir) > 0 ? 1.0 : -1.0;
    scattered = Ray(hit.p + hit.normal * EPS * side, dir);
    attenuation = mat.albedo;
    return true;
}

bool scatterEmissive(in Material mat, in Ray ray, in Hit hit, out vec3 attenuation, out Ray scattered, inout vec3 seed) {
    attenuation = mat.albedo * mat.intensity;
    return false;
}

bool scatterAnimated(in Material mat, in Ray ray, in Hit hit, out vec3 attenuation, out Ray scattered, inout vec3 seed) {
    vec2 p = hit.p.xz;
    float scale = 0.5;
    // float t = sin(ubo.time)*0.5+0.5;
    vec2 ip = round(p / scale);
    // vec3 color = mix(vec3(0.8, 0.6, 0.2), vec3(0.2, 0.3, 0.5), t);
    vec3 color = vec3(0.2, 0.3, 0.5);

    if (int(ip.x + ip.y + 1) % 2 == 0) {
        return scatterLambertian(LAMBERTIAN_MATERIAL(color*0.6), ray, hit, attenuation, scattered, seed);
        // return scatterMetal(METAL_MATERIAL(color, 0.2), ray, hit, attenuation, scattered, seed);
    } else {
        return scatterLambertian(LAMBERTIAN_MATERIAL(color), ray, hit, attenuation, scattered, seed);
    }
}

bool scatter(in Material mat, in Ray ray, in Hit hit, out vec3 attenuation, out Ray scattered, inout vec3 seed) {
    if (mat.type == mat_Lambertian)
        return scatterLambertian(mat, ray, hit, attenuation, scattered, seed);
    if (mat.type == mat_Metal)
        return scatterMetal(mat, ray, hit, attenuation, scattered, seed);
    if (mat.type == mat_Dielectric)
        return scatterDielectric(mat, ray, hit, attenuation, scattered, seed);
    if (mat.type == mat_Emissive)
        return scatterEmissive(mat, ray, hit, attenuation, scattered, seed);
    if (mat.type == mat_Animated)
        return scatterAnimated(mat, ray, hit, attenuation, scattered, seed);
    return scatterLambertian(DEFAULT_MATERIAL, ray, hit, attenuation, scattered, seed);
}

Material getMaterial(in Hit hit) {
    if (hit.type == obj_Sphere)
        return ssbo.spheres[hit.idx].mat;
    else if (hit.type == obj_Plane)
        return planes[hit.idx].mat;
    return DEFAULT_MATERIAL;
}

// ================== INTERSECTION ==================
float raySphereIntersection(in Ray ray, in Sphere sphere) {
    vec3 p = sphere.center - ray.origin;
    float dp = dot(ray.dir, p);
    float c = dot(p, p) - sphere.radius*sphere.radius;
    float delta = dp*dp - c;
    if (delta < 0)
        return -1;

    float t1 = dp - sqrt(delta);
    if (t1 >= 0) return t1;

    float t2 = dp + sqrt(delta);
    if (t2 >= 0) return t2;
    
    return -1;
}

float rayPlaneIntersection(in Ray ray, in Plane plane) {
    float denom = dot(plane.normal, ray.dir);
    if (abs(denom) > EPS) {
        float t = dot(plane.point - ray.origin, plane.normal) / denom;
        if (t >= EPS) return t;
    }
    return -1;
}

// ================== TRACING ==================
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
    float t = abs(1./0.);
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

vec3 gammaCorrection(vec3 color) {
    return color;
    // return color*color;
    // return sqrt(color);
}

vec4 traceRay(in Camera camera, in Ray ray, inout vec3 seed) {
    Hit hit = intersection(ray);
    vec3 color = vec3(1);
    float id = -1;
    float factor = 1;

    int i = 0;
    for (; i < MAX_BOUNCE_DEPTH; i++) {
        if (hit.type != obj_None) {
            if (id < 0) {
                id = 0;
                if (hit.type == obj_Sphere)
                    id = float(hit.idx+1) / ssbo.sphereCount;
            }

            // color = vec3(1.0/hit.t); // depth
            // color = vec3(float(i)/ssbo.sphereCount); // sphere id
            // break;

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
            // color = vec3(0.0);
            color *= skyColor(ray.dir);
            break;
        }
    }
    if (i == MAX_BOUNCE_DEPTH)
        color = vec3(0.0);

    color = min(max(color, vec3(0)), vec3(1));
    return vec4(color, float(max(id, 0)));
}

void main() {
    vec3 seed = initSeed();

    Camera camera = Camera(ubo.cameraPos, ubo.cameraDir, vec3(0, 1, 0));

    pushPlane(Plane(vec3(0, -1, 0), vec3(0, 1, 0), ANIMATED_MATERIAL));

    vec2 uv = fragPos * 0.5 + 0.5;
    vec3 prevColor = texture(prevTex, uv).rgb;
    if (ubo.frameCount <= 1) prevColor = vec3(0.0);

    vec4 currColor = vec4(0);
    for (int i = 0; i < SAMPLES_PER_PIXEL; i++) {
        vec2 offset = vec2(rand(seed), rand(seed)) / ubo.screenSize;
        Ray ray = getRay(camera, fragPos + offset);
        vec4 color = traceRay(camera, ray, seed);
        currColor.xyz += color.xyz;
        currColor.w = color.w;  // No interpolation on the ID
    }
    currColor.xyz /= SAMPLES_PER_PIXEL;

    float frame = float(max(ubo.frameCount, 1));
    vec3 mixedColor = mix(prevColor, currColor.rgb, 1.0 / frame);
    outColor = vec4(mixedColor, currColor.a);
}
