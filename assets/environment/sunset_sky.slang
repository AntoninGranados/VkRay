import "script.slang";

struct SunsetSky : ISky {
    [Color] float3 zenithColor = float3(0.2, 0.1, 0.4);
    [Color] float3 horizonColor = float3(1.0, 0.4, 0.2);

    float3 evaluate(float3 dir) {
        float t = clamp(0.5 * (dir.y + 1.0), 0.0, 1.0);
        return lerp(horizonColor, zenithColor, t);
    }
}
