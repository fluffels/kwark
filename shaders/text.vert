#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location=0) in vec3 position;
layout (location=1) in vec4 inColor;

layout (location=0) out vec4 outColor;

const float fontSize = 4.f;

void main() {
    gl_Position = vec4(-1 + position.x / (1920.f / fontSize), -1 + position.y / (1080.f / fontSize), 0.000001, 1);
    outColor = inColor;
}
