#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform Push {
    vec3 color;
} push;

layout(binding=0) uniform Uniform {
    mat4x4 mvp;
} uniforms;

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inColor;
layout(location=2) in vec3 inNormal;

layout(location=0) out vec3 outColor;
layout(location=1) out float outDistance;
layout(location=2) out vec4 outNormal;

void main() {
    gl_Position = uniforms.mvp * vec4(inPosition, 1.0);
    outNormal = uniforms.mvp * vec4(inNormal, 0);
    outDistance = sqrt(gl_Position.x * gl_Position.x + gl_Position.y * gl_Position.y + gl_Position.z * gl_Position.z);
    outColor = inColor;
}
