#material : version(1)

#param float scale = 1.0 : min(0)
#param float3 rockColor = float3(0.86, 0.85, 0.82) : color
#param float rockRoughness = 0.05 : min(0), max(1)
#param float3 veinColor = float3(0.16, 0.15, 0.17) : color
#param float veinRoughness = 0.95 : min(0), max(1)

static const float3 stretch = float3(1.0, 0.25, 0.6);
static const float3 veinDir = float3(1.0, 0.0, 1.0);
static const float bendAmp = 0.2;
static const float bendFreq = 0.1;
static const float distort = 0.3;
static const float sharpness = 4.0;
static const float noiseNorm = 8.0;

void main() {
    float3 p = pos * scale * stretch;

    float bend = fractalNoise(p * bendFreq, 6, 2.0, 0.5) * noiseNorm;
    float detail = fractalNoise(p * 1.5, 10, 2.0, 0.5) * noiseNorm;

    float phase = dot(p, veinDir) + bendAmp * bend;
    float band = 0.5 + 0.5 * sin(phase * 3.14159 + detail * distort);
    float vein = pow(1.0 - band, sharpness);

    mat = Glossy(
        lerp(rockColor, veinColor, vein),
        lerp(rockRoughness, veinRoughness, vein),
        2.0
    );
}
