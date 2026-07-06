#ifndef SKY_GLSL
#define SKY_GLSL

#include "inputs.glsl"
#include "utils.glsl"

vec3 skyColor(vec3 dir) {
    float t = clamp(0.5*(dir.y + 1.0), 0.0, 1.0);
    vec3 zenith, horizon;

    switch (ubo.render.lightMode) {
        case lightMode_Day:
            zenith = vec3(0.5, 0.7, 1.0);
            horizon = vec3(1.0, 1.0, 1.0);
            break;
        case lightMode_Sunset:
            zenith = vec3(0.2, 0.1, 0.4);
            horizon = vec3(1.0, 0.4, 0.2);
            break;
        case lightMode_Night:
            zenith  = vec3(0.01, 0.01, 0.03);
            horizon = vec3(0.05, 0.05, 0.1);
            break;
        case lightMode_Empty:
            return vec3(0.0);
            // return vec3(0.005, 0.005, 0.01);
            break;
        default:
            return vec3(1.0, 0.0, 1.0);
            break;
    }

    return mix(horizon, zenith, t);
}

#endif
