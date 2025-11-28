#version 450

layout(location = 0) in vec2 aPosition;
layout(location = 0) out vec2 fragPos;

void main() {
    gl_Position = vec4(aPosition, 0.0, 1.0);
    fragPos = aPosition;
}
