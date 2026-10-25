import "script.slang";

struct SmoothRandom : IMaterial {
    IBsdf evaluate(inout SurfacePoint surface, float3 wo, inout RngState rng) {
        float3 pos = surface.position;
        int octave = 10;
        float lacunarity = 2.0;
        float gain = 0.5;
        float r = (fractalNoise(pos, octave, lacunarity, gain) + 1.0) / 2.0;
        return Diffuse(float3(r));
    }
}
