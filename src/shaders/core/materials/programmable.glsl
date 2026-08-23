#ifndef PROGRAMMABLE_GLSL
#define PROGRAMMABLE_GLSL

#include "../utils.glsl"
#include "../random.glsl"

#include "material_utils.glsl"
#include "diffuse.glsl"
#include "metal.glsl"
#include "glossy.glsl"

#define SCALE 1.0
#define FEATHER 0.02
#define SMALL_FEATHER 0.01

// #define PROGRAMMABLE_SMOOTH_RANDOM
// #define PROGRAMMABLE_GOLD_VEIN
#define PROGRAMMABLE_MARBLE
// #define PROGRAMMABLE_CHECKERBOARD
// #define PROGRAMMABLE_TEST_GRID
// #define PROGRAMMABLE_POINT_GRID

#define MATERIAL_NORMAL mat_makeGlossy(mat_albedo(mat), 0.1, 2.0)
#define MATERIAL_DARK   mat_makeGlossy(mat_albedo(mat) * 0.8, 0.1, 2.0)
#define MATERIAL_BLACK  mat_makeDiffuse(vec3(0.02))

vec2 planeCoords(in Hit hit) {
    if (hit.object.type == obj_Sphere) {
        float u = atan(hit.normal.z, hit.normal.x) / (2.0 * PI);
        float v = asin(clamp(hit.normal.y, -1.0, 1.0)) / PI;
        return vec2(u, v);
    }

    vec3 up = abs(hit.normal.z) < (1.0 - EPS) ? vec3(0.0, 0.0, 1.0) : vec3(0.0, 1.0, 0.0);
    vec3 t = normalize(cross(up, hit.normal));
    vec3 b = cross(hit.normal, t);
    return vec2(dot(hit.p, t), dot(hit.p, b));
}

float fade(float t) {
    return 6 * pow(t, 5) - 15 * pow(t, 4) + 10 * pow(t, 3);
}

// 3D gradient noise. For a 2D sample, just pass p.z = 0.
float perlinNoise(vec3 p) {
    vec3 cell = p / SCALE;
    vec3 f = fract(cell);
    ivec3 p0 = ivec3(floor(cell));

    uint s;
    s = initSeed(p0 + ivec3(0, 0, 0), 0); vec3 grad000 = randomInSphere(s);
    s = initSeed(p0 + ivec3(1, 0, 0), 0); vec3 grad100 = randomInSphere(s);
    s = initSeed(p0 + ivec3(0, 1, 0), 0); vec3 grad010 = randomInSphere(s);
    s = initSeed(p0 + ivec3(1, 1, 0), 0); vec3 grad110 = randomInSphere(s);
    s = initSeed(p0 + ivec3(0, 0, 1), 0); vec3 grad001 = randomInSphere(s);
    s = initSeed(p0 + ivec3(1, 0, 1), 0); vec3 grad101 = randomInSphere(s);
    s = initSeed(p0 + ivec3(0, 1, 1), 0); vec3 grad011 = randomInSphere(s);
    s = initSeed(p0 + ivec3(1, 1, 1), 0); vec3 grad111 = randomInSphere(s);

    float n000 = dot(grad000, f - vec3(0, 0, 0));
    float n100 = dot(grad100, f - vec3(1, 0, 0));
    float n010 = dot(grad010, f - vec3(0, 1, 0));
    float n110 = dot(grad110, f - vec3(1, 1, 0));
    float n001 = dot(grad001, f - vec3(0, 0, 1));
    float n101 = dot(grad101, f - vec3(1, 0, 1));
    float n011 = dot(grad011, f - vec3(0, 1, 1));
    float n111 = dot(grad111, f - vec3(1, 1, 1));

    float u = fade(f.x); float v = fade(f.y); float w = fade(f.z);

    float nx00 = mix(n000, n100, u);
    float nx10 = mix(n010, n110, u);
    float nx01 = mix(n001, n101, u);
    float nx11 = mix(n011, n111, u);

    float nxy0 = mix(nx00, nx10, v);
    float nxy1 = mix(nx01, nx11, v);

    return mix(nxy0, nxy1, w);
}

const mat3 NOISE_ROTATION = mat3(
    0.00,  0.80,  0.60,
    -0.80,  0.36, -0.48,
    -0.60, -0.48,  0.64
);

const float FBM_SIGMA = 0.115;

float fractalNoise(vec3 p, int octaves, float lacunarity, float gain) {
    float amplitude = 1.0;
    float maxAmplitude = 0.0;
    float sum = 0.0;
    vec3 q = NOISE_ROTATION * p;
    for (int i = 0; i < octaves; i++) {
        sum += amplitude * perlinNoise(q);
        maxAmplitude += amplitude;
        amplitude *= gain;
        q = NOISE_ROTATION * (q * lacunarity);
    }
    return sum / (maxAmplitude * FBM_SIGMA);
}

float turbulence(vec3 p, int octaves, float lacunarity, float gain) {
    float amplitude = 1.0;
    float maxAmplitude = 0.0;
    float sum = 0.0;
    vec3 q = NOISE_ROTATION * p;
    for (int i = 0; i < octaves; i++) {
        sum += amplitude * abs(perlinNoise(q));
        maxAmplitude += amplitude;
        amplitude *= gain;
        q = NOISE_ROTATION * (q * lacunarity);
    }
    return sum / (maxAmplitude * 0.16);
}

float veinMaskAt(vec3 p) {
    float veinField = fractalNoise(p * 1.5, 3, 2.0, 0.5);
    return 1.0 - smoothstep(0.0, 0.45, abs(veinField));
}

Material createProgrammableMaterial(in Material mat, inout Hit hit) {
    vec2 p = planeCoords(hit);

#if defined(PROGRAMMABLE_SMOOTH_RANDOM)
    int octave = 10;
    float lacunarity = 2.0;
    float gain = 0.5;
    float r = (fractalNoise(hit.p, octave, lacunarity, gain) + 1) / 2;
    return mat_makeDiffuse(vec3(r));

#elif defined(PROGRAMMABLE_GOLD_VEIN)
    vec3 pos = hit.p * SCALE;
    float h = 0.0001;

    int rockOctaves = 15;
    float rockLacunarity = 2.0;
    float rockGain = 0.5;
    float height  = (fractalNoise(pos, rockOctaves, rockLacunarity, rockGain) + 1) / 2;
    float heightX = (fractalNoise(pos + vec3(h, 0, 0), rockOctaves, rockLacunarity, rockGain) + 1) / 2;
    float heightY = (fractalNoise(pos + vec3(0, h, 0), rockOctaves, rockLacunarity, rockGain) + 1) / 2;
    float heightZ = (fractalNoise(pos + vec3(0, 0, h), rockOctaves, rockLacunarity, rockGain) + 1) / 2;
    vec3 rockGradient = vec3(heightX - height, heightY - height, heightZ - height) / h;

    float veinMask  = veinMaskAt(pos);
    float veinMaskX = veinMaskAt(pos + vec3(h, 0, 0));
    float veinMaskY = veinMaskAt(pos + vec3(0, h, 0));
    float veinMaskZ = veinMaskAt(pos + vec3(0, 0, h));
    vec3 veinGradient = -vec3(veinMaskX - veinMask, veinMaskY - veinMask, veinMaskZ - veinMask) / h;

    vec3 rockColor = vec3(0.8);
    float rockRoughness = 1.0;
    vec3 goldColor = vec3(0.8, 0.7, 0.35);
    float goldRoughness = 0.1;
    float veinRecessDepth = 0.03;

    float bumpStrength = rockRoughness + step(0.5, veinMask) * (goldRoughness - rockRoughness);
    vec3 grad = rockGradient * bumpStrength + veinGradient * veinRecessDepth;
    vec3 tangentGrad = grad - dot(grad, hit.normal) * hit.normal;
    hit.normal = normalize(hit.normal - tangentGrad);

    if (veinMask > 0.5) {
        return mat_makeMetal(goldColor, goldRoughness);
    }
    return mat_makeDiffuse(rockColor);

#elif defined(PROGRAMMABLE_MARBLE)
    const vec3  stretch   = vec3(1.0, 0.25, 0.6);
    const vec3  veinDir   = vec3(1.0, 0.0, 1.0);
    const float bendAmp   = 0.2;
    const float bendFreq  = 0.1;
    const float distort   = 0.3;
    const float sharpness = 4.0;
    const float NOISE_NORM = 8.0;

    vec3 pos = hit.p * SCALE * stretch;

    float bend   = fractalNoise(pos * bendFreq,  6, 2.0, 0.5) * NOISE_NORM;
    float detail = fractalNoise(pos * 1.5,      10, 2.0, 0.5) * NOISE_NORM;

    float phase = dot(pos, veinDir) + bendAmp * bend;
    float band  = 0.5 + 0.5 * sin(phase * 3.14159 + detail * distort);
    float vein  = pow(1.0 - band, sharpness);

    vec3 rockColor = vec3(0.86, 0.85, 0.82);
    float rockRoughness = 0.05;
    vec3 veinColor = vec3(0.16, 0.15, 0.17);
    float veinRoughness = 0.95;

    float t = vein;
    return mat_makeGlossy(
        mix(rockColor, veinColor, vein), 
        mix(rockRoughness, veinRoughness, vein),
        2
    );

    // int veinOctaves = 5;
    // float veinLacunarity = 2.0;
    // float veinGain = 0.5;
    // float t = (turbulence(pos, veinOctaves, veinLacunarity, veinGain) + 1) / 2;
    
    // float band = 0.5 + 0.5 * sin(pos.y * PI + t * 6.0);
    // float vein = pow(1.0 - band, 6.0);

    // vec3 rockColor = vec3(1.0);
    // float rockRoughness = 0.1;
    // vec3 veinColor = vec3(0.0);
    // float veinRoughness = 0.9;


#elif defined(PROGRAMMABLE_CHECKERBOARD)
    if (int(round(p.x / SCALE) + round(p.y / SCALE) + 1) % 2 == 0) {
        return MATERIAL_BLACK;
        return MATERIAL_NORMAL;
    } else {
    }

#elif defined(PROGRAMMABLE_TEST_GRID)
    if (abs(p.x - round(p.x / SCALE) * SCALE) < FEATHER || abs(p.y - round(p.y / SCALE) * SCALE) < FEATHER) {
        return MATERIAL_BLACK;
    } else if (int(round(p.x / SCALE + 0.5) + round(p.y / SCALE + 0.5) + 1) % 2 == 0) {
        if (abs(p.x - round(p.x / SCALE * 2) * SCALE / 2) < SMALL_FEATHER || abs(p.y - round(p.y / SCALE * 2) * SCALE / 2) < SMALL_FEATHER) {
            return MATERIAL_BLACK;
        }
        return MATERIAL_NORMAL;
    }
    return MATERIAL_DARK;

#elif defined(PROGRAMMABLE_POINT_GRID)
    float x = abs(p.x - round(p.x / SCALE) * SCALE);
    float y = abs(p.y - round(p.y / SCALE) * SCALE);
    if (x*x + y*y < 0.08*0.08) {
        return MATERIAL_NORMAL;
    }
    return MATERIAL_BLACK;

#endif
}

#endif // PROGRAMMABLE_GLSL
