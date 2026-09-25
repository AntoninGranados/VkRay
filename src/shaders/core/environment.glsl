#ifndef ENVIRONMENT_GLSL
#define ENVIRONMENT_GLSL

#include "inputs.glsl"
#include "utils.glsl"
#include "../generated/sky_dispatch.glsl"

vec3 skyColor(vec3 dir) {
    if (ubo.render.skyParamsBase >= 0)
        return dispatchSky(ubo.render.skyParamsBase, dir);
    return vec3(0.1);
}

#endif
