#ifndef MATERIAL_UTILS_GLSL
#define MATERIAL_UTILS_GLSL

#include "../utils.glsl"

struct Material {
    int  type;
    uint base;
};

struct ResolvedMaterial {
    int   type;
    float payload[12];
};

void setAlbedo(inout ResolvedMaterial m, vec3 v) {
    m.payload[0] = v.r; m.payload[1] = v.g; m.payload[2] = v.b;
}

ResolvedMaterial Diffuse(vec3 albedo) {
    ResolvedMaterial m;
    m.type = mat_Diffuse;
    m.payload[0] = albedo.r; m.payload[1] = albedo.g; m.payload[2] = albedo.b;
    return m;
}

ResolvedMaterial Glossy(vec3 albedo, float roughness, float ior) {
    ResolvedMaterial m;
    m.type = mat_Glossy;
    m.payload[0] = albedo.r; m.payload[1] = albedo.g; m.payload[2] = albedo.b;
    glossyRoughness(m) = roughness;
    glossyIor(m) = ior;
    return m;
}

ResolvedMaterial Metal(vec3 albedo, float roughness) {
    ResolvedMaterial m;
    m.type = mat_Metal;
    m.payload[0] = albedo.r; m.payload[1] = albedo.g; m.payload[2] = albedo.b;
    metalRoughness(m) = roughness;
    return m;
}

ResolvedMaterial Dielectric(vec3 albedo, float roughness, float ior, float transmission, float density, float anisotropic) {
    ResolvedMaterial m;
    m.type = mat_Dielectric;
    m.payload[0] = albedo.r; m.payload[1] = albedo.g; m.payload[2] = albedo.b;
    dielectricRoughness(m) = roughness;
    dielectricIor(m) = ior;
    dielectricTransmission(m) = transmission;
    dielectricDensity(m) = density;
    dielectricAnisotropic(m) = anisotropic;
    dielectricPriority(m) = 0.0;
    return m;
}

// TODO: implement all parameters
ResolvedMaterial Principled(vec3 albedo, float roughness, float metalness, float ior, float alpha) {
    ResolvedMaterial m;
    m.type = mat_Principled;
    m.payload[0] = albedo.r; m.payload[1] = albedo.g; m.payload[2] = albedo.b;
    principledRoughness(m) = roughness;
    principledMetalness(m) = metalness;
    principledIor(m) = ior;
    principledTransmission(m) = 0.0;
    principledDensity(m) = 0.0;
    principledAnisotropic(m) = 0.0;
    principledAlpha(m) = alpha;
    principledPriority(m) = 0.0;
    return m;
}

// ============== BSDF ==============
struct BSDFMediumInfo {
    bool isDielectric;
    bool isVolume;
    vec3 absorption;
    float density;
    float scatterAlbedo;
    float anisotropic;   // `g` in the Henyey–Greenstein phase function (https://en.wikipedia.org/wiki/Henyey–Greenstein_phase_function)
};

struct BSDFSample {
    vec3 weight;
    vec3 wi;
    float pdf;
    bool isDelta;
    BSDFMediumInfo medium;
};

struct BSDFEval {
    vec3 f;
    float pdf;
};

#define DEFAULT_MATERIAL unpackMaterial(materialBuffer.materials[0])
#define NO_MEDIUM BSDFMediumInfo(false, false, vec3(1.0), 0.0, 0.0, 0.0)

// ============== REFRACTION/REFLECTION ==============
#define SCHLICK_APPROX(cosine, F0) F0 + (1-F0) * pow((1 - cosine), 5)

vec3 schlickIoR(float cosine, float ri) {
    float F0 = (1 - ri) / (1 + ri);
    F0 = F0*F0;
    return SCHLICK_APPROX(cosine, vec3(F0));
}

float fresnelDielectric(float cosI, float etaI, float etaT) {
    float sinT2 = (etaI / etaT) * (etaI / etaT) * (1.0 - cosI * cosI);
    if (sinT2 >= 1.0) return 1.0;
    float cosT = sqrt(1.0 - sinT2);
    float Rs = (etaI * cosI - etaT * cosT) / (etaI * cosI + etaT * cosT);
    float Rp = (etaT * cosI - etaI * cosT) / (etaT * cosI + etaI * cosT);
    return (Rs * Rs + Rp * Rp) * 0.5;
}

vec3 schlickAlbedo(float cosine, vec3 albedo) {
    return SCHLICK_APPROX(cosine, albedo);
}

bool isTransmissive(in ResolvedMaterial mat) {
    if (mat.type == mat_Dielectric) return true;
    if (mat.type == mat_Principled) return (1.0 - principledMetalness(mat)) * principledTransmission(mat) > EPS;
    return false;
}

bool isMediumBoundary(in ResolvedMaterial mat) {
    return mat.type == mat_Volume || isTransmissive(mat);
}

int mediumPriority(in ResolvedMaterial mat) {
    switch (mat.type) {
        case mat_Dielectric: return int(dielectricPriority(mat));
        case mat_Principled: return int(principledPriority(mat));
        case mat_Volume:     return int(volumePriority(mat));
        default:             return 0;
    }
}

struct MediumEntry {
    uint objectKey;
    int priority;
    float ior;
    vec3 absorption;
    float density;
    float scatterAlbedo;
    float anisotropic;
};

uint mediumObjectKey(in Object object) {
    return (uint(object.type) << 24) | object.id;
}

#define MEDIUM_STACK_SIZE 4

MediumEntry mediumStack[MEDIUM_STACK_SIZE];
int mediumStackDepth = 0;

void resetMediumStack() {
    mediumStackDepth = 0;
}

void pushMedium(in MediumEntry entry) {
    if (mediumStackDepth == MEDIUM_STACK_SIZE) {
        for (int i = 1; i < MEDIUM_STACK_SIZE; i++) mediumStack[i - 1] = mediumStack[i];
        mediumStackDepth--;
    }
    mediumStack[mediumStackDepth] = entry;
    mediumStackDepth++;
}

int findMedium(in Object object) {
    uint objectKey = mediumObjectKey(object);
    for (int i = mediumStackDepth - 1; i >= 0; i--)
        if (mediumStack[i].objectKey == objectKey) return i;
    return -1;
}

void removeMedium(in Object object) {
    int index = findMedium(object);
    if (index < 0) return;
    for (int i = index + 1; i < mediumStackDepth; i++) mediumStack[i - 1] = mediumStack[i];
    mediumStackDepth--;
}

int activeMediumIndex(int excluded) {
    int activeIndex = -1;
    for (int i = 0; i < mediumStackDepth; i++) {
        if (i == excluded) continue;
        if (activeIndex < 0 || mediumStack[i].priority >= mediumStack[activeIndex].priority) activeIndex = i;
    }
    return activeIndex;
}

float activeIor(int excluded) {
    int activeIndex = activeMediumIndex(excluded);
    return activeIndex < 0 ? 1.0 : mediumStack[activeIndex].ior;
}

BSDFMediumInfo activeMedium() {
    int activeIndex = activeMediumIndex(-1);
    if (activeIndex < 0) return NO_MEDIUM;
    return BSDFMediumInfo(
        false, mediumStack[activeIndex].density > 0.0, mediumStack[activeIndex].absorption,
        mediumStack[activeIndex].density, mediumStack[activeIndex].scatterAlbedo, mediumStack[activeIndex].anisotropic
    );
}

MediumEntry makeMediumEntry(in ResolvedMaterial mat, in Object object) {
    uint objectKey = mediumObjectKey(object);
    switch (mat.type) {
        case mat_Dielectric:
            return MediumEntry(
                objectKey, mediumPriority(mat), dielectricIor(mat), albedo(mat),
                dielectricDensity(mat), dielectricTransmission(mat), dielectricAnisotropic(mat)
            );
        case mat_Principled:
            return MediumEntry(
                objectKey, mediumPriority(mat), principledIor(mat), albedo(mat),
                principledDensity(mat), 0.0, principledAnisotropic(mat)
            );
        default:
            return MediumEntry(
                objectKey, mediumPriority(mat), activeIor(-1), albedo(mat),
                volumeDensity(mat), 1.0, volumeAnisotropic(mat)
            );
    }
}

bool isFalseInterface(in ResolvedMaterial mat, in Hit hit) {
    if (hit.frontFace) {
        int activeIndex = activeMediumIndex(-1);
        return activeIndex >= 0 && mediumPriority(mat) < mediumStack[activeIndex].priority;
    }
    int index = findMedium(hit.object);
    return index >= 0 && index != activeMediumIndex(-1);
}

void interfaceIors(in Hit hit, float ior, out float etaI, out float etaT) {
    if (hit.frontFace) {
        etaI = activeIor(-1);
        etaT = ior;
    } else {
        etaI = ior;
        etaT = activeIor(findMedium(hit.object));
    }
}

void crossInterface(in ResolvedMaterial mat, in Hit hit) {
    if (hit.frontFace) pushMedium(makeMediumEntry(mat, hit.object));
    else removeMedium(hit.object);
}

// ============== MIRROR ==============
BSDFEval evalMirrorBSDF(in vec3 albedo, in Hit hit, in vec3 wo, in vec3 wi) {
    float VoN = max(dot(wo, hit.normal), 0.0);
    return BSDFEval(
        albedo * schlickIoR(VoN, 0.0) / VoN,
        0.0
    );
}

BSDFSample sampleMirrorBSDF(in vec3 albedo, in Hit hit, in vec3 wo) {
    vec3 wi = reflect(-wo, hit.normal);
    float cosB = abs(dot(hit.normal, wi));
    BSDFEval eval = evalMirrorBSDF(albedo, hit, wo, wi);
    
    BSDFSample bsdf;
    bsdf.wi = wi;
    bsdf.weight = eval.f * cosB;
    bsdf.pdf = eval.pdf;
    bsdf.isDelta = true;
    bsdf.medium.isDielectric = false;
    return bsdf;
}

#endif // MATERIAL_UTILS_GLSL
