__exported import "../utils.slang";

// ============== BSDF ==============
struct BSDFMediumInfo {
    bool isDielectric;
    bool isVolume;
    float3 absorption;
    float density;
    float scatterAlbedo;
    float anisotropic;   // `g` in the Henyey–Greenstein phase function (https://en.wikipedia.org/wiki/Henyey–Greenstein_phase_function)
};

struct BSDFSample {
    float3 weight;
    float3 wi;
    float pdf;
    bool isDelta;
    BSDFMediumInfo medium;
};

struct BSDFEval {
    float3 f;
    float pdf;
};

static const BSDFMediumInfo NO_MEDIUM = BSDFMediumInfo(false, false, float3(1.0), 0.0, 0.0, 0.0);

// ============== REFRACTION/REFLECTION ==============
float3 schlickApprox(float cosine, float3 F0) {
    return F0 + (1 - F0) * pow((1 - cosine), 5);
}

float3 schlickIoR(float cosine, float ri) {
    float F0 = (1 - ri) / (1 + ri);
    F0 = F0*F0;
    return schlickApprox(cosine, float3(F0));
}

float fresnelDielectric(float cosI, float etaI, float etaT) {
    float sinT2 = (etaI / etaT) * (etaI / etaT) * (1.0 - cosI * cosI);
    if (sinT2 >= 1.0) return 1.0;
    float cosT = sqrt(1.0 - sinT2);
    float Rs = (etaI * cosI - etaT * cosT) / (etaI * cosI + etaT * cosT);
    float Rp = (etaT * cosI - etaI * cosT) / (etaT * cosI + etaI * cosT);
    return (Rs * Rs + Rp * Rp) * 0.5;
}

float3 schlickAlbedo(float cosine, float3 albedo) {
    return schlickApprox(cosine, albedo);
}

struct MediumEntry {
    uint objectKey;
    int priority;
    float ior;
    float3 absorption;
    float density;
    float scatterAlbedo;
    float anisotropic;
};

uint mediumObjectKey(in Object object) {
    return (uint(object.type) << 24) | object.id;
}

static const int MEDIUM_STACK_SIZE = 16;

static MediumEntry mediumStack[MEDIUM_STACK_SIZE];
static int mediumStackDepth = 0;

void resetMediumStack() {
    mediumStackDepth = 0;
}

void pushMedium(MediumEntry entry) {
    if (mediumStackDepth == MEDIUM_STACK_SIZE) {
        for (int i = 1; i < MEDIUM_STACK_SIZE; i++) mediumStack[i - 1] = mediumStack[i];
        mediumStackDepth--;
    }
    mediumStack[mediumStackDepth] = entry;
    mediumStackDepth++;
}

int findMedium(Object object) {
    uint objectKey = mediumObjectKey(object);
    for (int i = mediumStackDepth - 1; i >= 0; i--)
        if (mediumStack[i].objectKey == objectKey) return i;
    return -1;
}

void removeMedium(Object object) {
    int index = findMedium(object);
    if (index < 0) return;
    for (int i = index + 1; i < mediumStackDepth; i++) mediumStack[i - 1] = mediumStack[i];
    mediumStackDepth--;
}

int activeMediumIndex(int excluded) {
    int index = -1;
    for (int i = 0; i < mediumStackDepth; i++) {
        if (i == excluded) continue;
        if (index < 0 || mediumStack[i].priority >= mediumStack[index].priority) index = i;
    }
    return index;
}

float activeIor(int excluded) {
    int index = activeMediumIndex(excluded);
    return index < 0 ? 1.0 : mediumStack[index].ior;
}

BSDFMediumInfo activeMedium() {
    int index = activeMediumIndex(-1);
    if (index < 0) return NO_MEDIUM;
    return BSDFMediumInfo(
        false, mediumStack[index].density > 0.0, mediumStack[index].absorption,
        mediumStack[index].density, mediumStack[index].scatterAlbedo, mediumStack[index].anisotropic
    );
}

bool isFalseInterface(int priority, Hit hit) {
    if (hit.frontFace) {
        int index = activeMediumIndex(-1);
        return index >= 0 && priority < mediumStack[index].priority;
    }
    int index = findMedium(hit.object);
    return index >= 0 && index != activeMediumIndex(-1);
}

void interfaceIors(Hit hit, float ior, out float etaI, out float etaT) {
    if (hit.frontFace) {
        etaI = activeIor(-1);
        etaT = ior;
    } else {
        etaI = ior;
        etaT = activeIor(findMedium(hit.object));
    }
}

void crossInterface(MediumEntry entry, Hit hit) {
    if (hit.frontFace) pushMedium(entry);
    else removeMedium(hit.object);
}

// ============== MIRROR ==============
BSDFEval evalMirrorBSDF(in float3 albedo, in Hit hit, in float3 wo, in float3 wi) {
    float VoN = max(dot(wo, hit.normal), 0.0);
    return BSDFEval(
        albedo * schlickIoR(VoN, 0.0) / VoN,
        0.0
    );
}

BSDFSample sampleMirrorBSDF(in float3 albedo, in Hit hit, in float3 wo) {
    float3 wi = reflect(-wo, hit.normal);
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
