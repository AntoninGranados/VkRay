#ifndef APERTURE_GLSL
#define APERTURE_GLSL

#include "../random/utils.glsl"

#ifdef CAMERA_LENS_APERTURE
#include "../inputs.glsl"

vec2 sampleLens(inout RngState rng) {
    for (int i = 0; i < 16; i++) {
        vec2 p = vec2(rand(rng), rand(rng)) * 2.0 - 1.0;
        float mask = texture(lensSampler, p * 0.5 + 0.5).r;
        if (rand(rng) < mask) return p;
    }
    return vec2(0.0);
}
#else
vec2 sampleLens(inout RngState rng) {
    return vec2(0.0);
}
#endif

#endif
