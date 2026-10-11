#sky: version(1)

#param float3 zenithColor = float3(0.2, 0.1, 0.4): color
#param float3 horizonColor = float3(1.0, 0.4, 0.2): color

void main() {
    float t = clamp(0.5 * (dir.y + 1.0), 0.0, 1.0);
    result = lerp(horizonColor, zenithColor, t);
}
