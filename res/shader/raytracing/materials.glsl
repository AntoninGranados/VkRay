#ifndef MATERIALS_GLSL
#define MATERIALS_GLSL

#include "utils.glsl"
#include "random.glsl"

Material makeMaterial(Enum type, vec3 albedo, float f0, float f1) {
    Material m;
    m.type = type;
    m.albedo = albedo;
    m.payload[0] = f0;
    m.payload[1] = f1;
    return m;
}

#define SET_MATERIAL_0(type, albedo)         makeMaterial(type, albedo, 0.0, 0.0)
#define SET_MATERIAL_1(type, albedo, f0)     makeMaterial(type, albedo, f0, 0.0)
#define SET_MATERIAL_2(type, albedo, f0, f1) makeMaterial(type, albedo, f0, f1)

#define DEFAULT_MATERIAL                     SET_MATERIAL_0(mat_Lambertian, vec3(1,0,1)*0.7)
#define LAMBERTIAN_MATERIAL(albedo)          SET_MATERIAL_0(mat_Lambertian, albedo)
#define METAL_MATERIAL(albedo, fuzz)         SET_MATERIAL_1(mat_Metal, albedo, fuzz)
#define DIELECTRIX_MATERIAL(albedo, ior)     SET_MATERIAL_1(mat_Dielectric, albedo, ior)
#define EMISSIVE_MATERIAL(albedo, intensity) SET_MATERIAL_1(mat_Emissive, albedo, intensity)
#define GLOSSY_MATERIAL(albedo, ior, fuzz)   SET_MATERIAL_2(mat_Glossy, albedo, ior, fuzz)
#define CHECKERBOARD_MATERIAL                SET_MATERIAL_1(mat_Checkerboard, vec3(0), 2)

#define metalFuzz(mat) mat.payload[0]
#define dielectricIoR(mat) mat.payload[0]
#define dielectricFuzz(mat) mat.payload[1]
#define emissiveIntensity(mat) mat.payload[0]
#define glossyIoR(mat) mat.payload[0]
#define glossyFuzz(mat) mat.payload[1]
#define checkerboardScale(mat) mat.payload[0]

float schlick_approx(float cosine, float ri) {
    float r0 = (1 - ri) / (1 + ri);
    r0 = r0*r0;
    return r0 + (1-r0) * pow((1 - cosine),5);
}

struct ScatterResult {
    vec3 attenuation;
    Ray scattered;
    bool isScattered;
    bool isDiffuse;
};

// ================== SCATTERING FUNCIONS ==================
void scatterLambertian(in Material mat, in Ray ray, in Hit hit, out ScatterResult result, inout uint seed) {
    vec3 dir = hit.normal + randomInSphere(seed);
    if (length(dir) < EPS) dir = hit.normal;
    dir = normalize(dir);

    result.scattered = Ray(hit.p + hit.normal * EPS, dir);
    result.attenuation = mat.albedo / PI;
    result.isScattered = true;
    result.isDiffuse = true;
}

void scatterMetal(in Material mat, in Ray ray, in Hit hit, out ScatterResult result, inout uint seed) {
    vec3 dir = reflect(ray.dir, hit.normal);
    dir = normalize(dir + randomInSphere(seed) * metalFuzz(mat));

    result.scattered = Ray(hit.p + hit.normal * EPS, dir);
    result.attenuation = mat.albedo;
    result.isScattered = true;
    result.isDiffuse = false;
}

void scatterDielectric(in Material mat, in Ray ray, in Hit hit, out ScatterResult result, inout uint seed) {
    float ri = hit.front_face ? (1.0/dielectricIoR(mat)) : dielectricIoR(mat);

    float cos_theta = min(dot(-ray.dir, hit.normal), 1.0);
    float sin_theta = sqrt(1.0 - cos_theta*cos_theta);

    vec3 dir;
    if (ri * sin_theta > 1 || schlick_approx(cos_theta, ri) > rand(seed)) {
        dir = reflect(ray.dir, hit.normal);
        dir = normalize(dir + randomInSphere(seed) * dielectricFuzz(mat));
    } else {
        dir = refract(ray.dir, hit.normal, ri);
        dir = normalize(dir + randomInSphere(seed) * dielectricFuzz(mat));
    }

    float side = dot(hit.normal, dir) > 0 ? 1.0 : -1.0;
    result.scattered = Ray(hit.p + hit.normal * EPS * side, dir);
    result.attenuation = mat.albedo;
    result.isScattered = true;
    result.isDiffuse = false;
}

void scatterEmissive(in Material mat, in Ray ray, in Hit hit, out ScatterResult result, inout uint seed) {
    result.attenuation = mat.albedo * emissiveIntensity(mat);
    result.isScattered = false;
}

void scatterGlossy(in Material mat, in Ray ray, in Hit hit, out ScatterResult result, inout uint seed) {
    float ri = hit.front_face ? (1.0/glossyIoR(mat)) : glossyIoR(mat);

    float cos_theta = min(dot(-ray.dir, hit.normal), 1.0);

    if (schlick_approx(cos_theta, ri) > rand(seed))
        scatterMetal(METAL_MATERIAL(vec3(1.0), glossyFuzz(mat)), ray, hit, result, seed);
    else
        scatterLambertian(LAMBERTIAN_MATERIAL(mat.albedo), ray, hit, result, seed);
}

void scatterCheckerboard(in Material mat, in Ray ray, in Hit hit, out ScatterResult result, inout uint seed) {
    vec2 p = hit.p.xz;
    vec2 ip = round(p / checkerboardScale(mat));
    vec3 color = vec3(0.2, 0.3, 0.5);

    // Artificial gutters on tile edges //! this is absolute magic
    float s = 6.0;
    vec2 uv = p / checkerboardScale(mat);
    vec2 tile_uv = fract(uv - 0.5);
    vec2 centered = tile_uv - 0.5;
    vec2 edge_dist = 0.5 - abs(centered);
    float gutter_width = 0.25 / s;
    vec2 border = step(edge_dist, vec2(gutter_width));
    if (border.x > 0.0 || border.y > 0.0) {
        vec2 falloff = clamp((gutter_width - edge_dist) * (1.0 / gutter_width), 0.0, 1.0) * border;
        vec2 normal_offset = sign(centered) * falloff;
        float len2 = dot(normal_offset, normal_offset);
        if (len2 > 0.0) {
            normal_offset *= inversesqrt(max(len2, EPS)) * 0.85;
            hit.normal = normalize(hit.normal + vec3(normal_offset.x, 0.0, normal_offset.y));
        }
    }

    if (border.x > 0.0 || border.y > 0.0) {
        scatterLambertian(LAMBERTIAN_MATERIAL(color*0.4), ray, hit, result, seed);
    } else if (int(ip.x + ip.y + 1) % 2 == 0) {
        scatterMetal(METAL_MATERIAL(color*0.6, 0.01), ray, hit, result, seed);
    } else {
        scatterLambertian(LAMBERTIAN_MATERIAL(color), ray, hit, result, seed);
    }
}

void scatter(in Material mat, in Ray ray, in Hit hit, out ScatterResult result, inout uint seed) {
    switch (mat.type) {
        case mat_Lambertian:   scatterLambertian(mat, ray, hit, result, seed); break;
        case mat_Metal:        scatterMetal(mat, ray, hit, result, seed); break;
        case mat_Dielectric:   scatterDielectric(mat, ray, hit, result, seed); break;
        case mat_Emissive:     scatterEmissive(mat, ray, hit, result, seed); break;
        case mat_Glossy:       scatterGlossy(mat, ray, hit, result, seed); break;
        case mat_Checkerboard: scatterCheckerboard(mat, ray, hit, result, seed); break;
        default:               scatterLambertian(DEFAULT_MATERIAL, ray, hit, result, seed); break;
    }
}

#endif
