#ifndef MATERIALS_GLSL
#define MATERIALS_GLSL

#include "utils.glsl"
#include "random.glsl"

#define mat_Lambertian  Enum(0)
#define mat_Metal       Enum(1)
#define mat_Dielectric  Enum(2)
#define mat_Emissive    Enum(3)
#define mat_Animated    Enum(4)
struct Material {
    Enum type;
    vec3 albedo;
    float fuzz;
    float refraction_index;
    float intensity;
};

#define DEFAULT_MATERIAL                     Material(mat_Lambertian, vec3(1,0,1)*0.7, 0.0, 0.0, 0.0)
#define LAMBERTIAN_MATERIAL(albedo)          Material(mat_Lambertian, albedo, 0.0, 0.0, 0.0)
#define METAL_MATERIAL(albedo, fuzz)         Material(mat_Metal, albedo, fuzz, 0.0, 0.0)
#define DIELECTRIX_MATERIAL(albedo, index)   Material(mat_Dielectric, albedo, 0.0, index, 0.0)
#define EMISSIVE_MATERIAL(albedo, intensity) Material(mat_Emissive, albedo, 0.0, 0.0, intensity)
#define ANIMATED_MATERIAL                    Material(mat_Animated, vec3(0), 0.0, 0.0, 0.0)

// ================== SCATTERING FUNCIONS ==================
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

    // Artificial gutters on tile edges //! this is absolute magic
    float s = 6.0;
    vec2 uv = p / scale;
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
        return scatterLambertian(LAMBERTIAN_MATERIAL(color*0.4), ray, hit, attenuation, scattered, seed);
    } else if (int(ip.x + ip.y + 1) % 2 == 0) {
        // return scatterLambertian(LAMBERTIAN_MATERIAL(color*0.6), ray, hit, attenuation, scattered, seed);
        return scatterMetal(METAL_MATERIAL(color*0.6, 0.01), ray, hit, attenuation, scattered, seed);
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


#endif
