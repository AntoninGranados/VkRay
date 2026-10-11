__exported import "basics.slang";
__exported import "perlin.slang";
__exported import "simplex.slang";
__exported import "voronoi.slang";
__exported import "value.slang";

static const float3x3 NOISE_ROTATION = float3x3(
    0.00,  0.80,  0.60,
    -0.80,  0.36, -0.48,
    -0.60, -0.48,  0.64
);

static const float FBM_SIGMA = 0.115 * PERLIN_NOISE_3D_NORM;
static const float TURBULENCE_SIGMA = 0.16 * PERLIN_NOISE_3D_NORM;

float fractalNoise(float3 p, int octaves, float lacunarity, float gain) {
    RngState rng = initRngState(0u);

    float amplitude = 1.0;
    float maxAmplitude = 0.0;
    float sum = 0.0;
    float3 q = mul(p, NOISE_ROTATION);
    for (int i = 0; i < octaves; i++) {
        sum += amplitude * (perlinNoise(q, rng).value * 2 - 1);
        maxAmplitude += amplitude;
        amplitude *= gain;
        q = mul(q * lacunarity, NOISE_ROTATION);
    }
    return sum / (maxAmplitude * FBM_SIGMA);
}

float turbulence(float3 p, int octaves, float lacunarity, float gain) {
    RngState rng = initRngState(0u);

    float amplitude = 1.0;
    float maxAmplitude = 0.0;
    float sum = 0.0;
    float3 q = mul(p, NOISE_ROTATION);
    for (int i = 0; i < octaves; i++) {
        sum += amplitude * abs(perlinNoise(q, rng).value * 2 - 1);
        maxAmplitude += amplitude;
        amplitude *= gain;
        q = mul(q * lacunarity, NOISE_ROTATION);
    }
    return sum / (maxAmplitude * TURBULENCE_SIGMA);
}
