// --------------- INPUTS ---------------
// float3 dir

// --------------- OUTPUTS ---------------
// float3 result

#sky: version(1)

#param float3 zenithColor = float3(0.5, 0.7, 1.0): color
#param float3 horizonColor = float3(1.0, 1.0, 1.0): color

void main() {
    float t = clamp(0.5 * (dir.y + 1.0), 0.0, 1.0);
    result = lerp(horizonColor, zenithColor, t);
}
