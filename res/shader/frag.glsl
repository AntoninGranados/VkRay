#version 450

layout(set = 0, binding = 0) uniform sampler2D tex;

layout(location = 0) in vec2 fragPos;
layout(location = 0) out vec4 outColor;

float hash13(vec3 p3) {
    p3  = fract(p3 * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float rand(inout vec3 seed) {
    float r = hash13(seed);
    seed += vec3(1.0, 1.0, 1.0);
    return r;
}

void main() {
    vec2 uv = fragPos * 0.5 + 0.5;
    vec3 curr = texture(tex, uv).rgb;
    outColor = vec4(curr, 1.0);
    
    // float id = texture(tex, uv).a;
    // vec3 i = vec3(id);
    // if (id == 0)
    //     outColor = vec4(vec3(0.0), 1.0);
    // else
    //     outColor = vec4(vec3(rand(i), rand(i), rand(i)), 1.0);
}
