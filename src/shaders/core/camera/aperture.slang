import "../random/utils.slang";

#ifdef CAMERA_LENS_APERTURE
Sampler2D lensSampler;

float2 sampleLens(inout RngState rng) {
    for (int i = 0; i < 16; i++) {
        float2 p = float2(rand(rng), rand(rng)) * 2.0 - 1.0;
        float mask = lensSampler.SampleLevel(p * 0.5 + 0.5, 0).r;
        if (rand(rng) < mask) return p;
    }
    return float2(0.0);
}
#else
float2 sampleLens(inout RngState rng) {
    return float2(0.0);
}
#endif
