#material : version(1)

#param float scale = 1.0 : min(0)
#param float3 rockColor = float3(0.8) : color
#param float rockRoughness = 1.0 : min(0), max(1)
#param float3 goldColor = float3(0.8, 0.7, 0.35) : color
#param float goldRoughness = 0.1 : min(0), max(1)
#param float veinRecessDepth = 0.03 : min(0), max(0.1)

void main() {
    float3 p = pos * scale;
    float h = 0.0001;

    int rockOctaves = 15;
    float rockLacunarity = 2.0;
    float rockGain = 0.5;
    float height  = (fractalNoise(p, rockOctaves, rockLacunarity, rockGain) + 1.0) / 2.0;
    float heightX = (fractalNoise(p + float3(h, 0, 0), rockOctaves, rockLacunarity, rockGain) + 1.0) / 2.0;
    float heightY = (fractalNoise(p + float3(0, h, 0), rockOctaves, rockLacunarity, rockGain) + 1.0) / 2.0;
    float heightZ = (fractalNoise(p + float3(0, 0, h), rockOctaves, rockLacunarity, rockGain) + 1.0) / 2.0;
    float3 rockGradient = float3(heightX - height, heightY - height, heightZ - height) / h;

    float veinMask  = 1.0 - smoothstep(0.0, 0.45, abs(fractalNoise(p * 1.5, 3, 2.0, 0.5)));
    float veinMaskX = 1.0 - smoothstep(0.0, 0.45, abs(fractalNoise((p + float3(h, 0, 0)) * 1.5, 3, 2.0, 0.5)));
    float veinMaskY = 1.0 - smoothstep(0.0, 0.45, abs(fractalNoise((p + float3(0, h, 0)) * 1.5, 3, 2.0, 0.5)));
    float veinMaskZ = 1.0 - smoothstep(0.0, 0.45, abs(fractalNoise((p + float3(0, 0, h)) * 1.5, 3, 2.0, 0.5)));
    float3 veinGradient = -float3(veinMaskX - veinMask, veinMaskY - veinMask, veinMaskZ - veinMask) / h;

    float bumpStrength = rockRoughness + step(0.5, veinMask) * (goldRoughness - rockRoughness);
    float3 grad = rockGradient * bumpStrength + veinGradient * veinRecessDepth;
    float3 tangentGrad = grad - dot(grad, normal) * normal;
    new_normal = normalize(normal - tangentGrad);

    if (veinMask > 0.5) {
        mat = Metal(goldColor, goldRoughness);
    } else {
        mat = Diffuse(rockColor);
    }
}
