__exported import "../utils.slang";
__exported import "hash.slang";
#ifdef RANDOM_BLUE_NOISE
import "blue_noise.slang";
#endif

struct RngState {
    uint seed;
    uint2 pixel;
    uint sampleIndex;
    uint dimension;
    uint dimensionEnd;
};

struct NoiseState2D {
    float value;
    float2 gradient;
};

struct NoiseState3D {
    float value;
    float3 gradient;
};

void offsetRngState(inout RngState rng, in RngState rngOffset) {
    rng.seed += rngOffset.seed;
}

RngState initRngState(uint seed) {
    return RngState(seed, uint2(0), 0u, 0u, 0u);
}

RngState initRngState(uint2 pos, uint frame) {
    uint v = pos.x + pos.y * 4096u + frame * 1315423911u;
    return initRngState(pcgHash(v));
}

RngState initRngState(uint3 pos, uint frame) {
    uint v = pos.x + pos.y * 4096u + pos.z * 4096u * 4096u + frame * 1315423911u;
    return initRngState(pcgHash(v));
}

RngState initBlueNoiseRngState(uint2 pixel, uint sampleIndex) {
    RngState rng = initRngState(pixel, sampleIndex);
    rng.pixel = pixel;
    rng.sampleIndex = sampleIndex;
    return rng;
}

void setRngDimensions(inout RngState rng, uint first, uint count) {
    rng.dimension = first;
    rng.dimensionEnd = first + count;
}

void hashRngState(inout RngState rng) {
    rng.seed = pcgHash(rng.seed);
}

static const uint UINT_MAX = 0xFFFFFFFFu;
float rand(inout RngState rng) {
#ifdef RANDOM_BLUE_NOISE
    if (rng.dimension < rng.dimensionEnd) {
        float shift = float(pcgHash(pcgHash(rng.sampleIndex) + rng.dimension)) / float(UINT_MAX);
        return frac(blueNoise(rng.pixel, rng.dimension++) + shift);
    }
#endif
    hashRngState(rng);
    return float(rng.seed) / float(UINT_MAX);
}

float2 rand2(inout RngState rng) {
    return float2(rand(rng), rand(rng));
}

float3 rand3(inout RngState rng) {
    return float3(rand2(rng), rand(rng));
}

float4 rand4(inout RngState rng) {
    return float4(rand3(rng), rand(rng));
}

float3 randomOnSphere(inout RngState rng) {
    float z = 1.0 - 2.0 * rand(rng);
    float r = sqrt(max(0.0, 1.0 - z*z));
    float phi = rand(rng) * 2 * PI;
    float x = r * cos(phi);
    float y = r * sin(phi);
    return float3(x, y, z);
}

float3 randomInBall(inout RngState rng) {
    float3 direction = randomOnSphere(rng);
    return direction * pow(rand(rng), 1.0 / 3.0);
}

float3 randomInHemisphere(inout RngState rng, float3 normal) {
    float3 v = randomOnSphere(rng);
    return dot(v, normal) < 0.0 ? -v : v;
}

float2 randomOnCircle(inout RngState rng) {
    float theta = 6.2831853 * rand(rng);
    float x = cos(theta);
    float y = sin(theta);
    return float2(x, y);
}

float2 randomInDisk(inout RngState rng) {
    float r  = sqrt(rand(rng));
    return randomOnCircle(rng) * r;
}
