#version 450

layout(set = 0, binding = 0) uniform sampler2D tex;

layout(location = 0) in vec2 fragPos;
layout(location = 0) out vec4 outColor;

void main() {
    vec2 uv = fragPos * 0.5 + 0.5;
    vec3 curr = texture(tex, uv).rgb;
    float id = texture(tex, uv).a;
    outColor = vec4(curr, 1.0);
}
