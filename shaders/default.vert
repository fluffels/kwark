#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform Push {
    vec3 color;
} push;

layout(binding=0) uniform Uniform {
    mat4x4 mvp;
} uniforms;

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec3 inLight;

layout(location=0) out float outDistance;
layout(location=1) out vec3 outNormal;
layout(location=2) out vec3 outLight;

void main() {
    gl_Position = uniforms.mvp * vec4(inPosition, 1.0);
    outDistance = sqrt(
        gl_Position.x * gl_Position.x +
        gl_Position.y * gl_Position.y +
        gl_Position.z * gl_Position.z
    );
    outNormal = inNormal;
    outLight = inLight;
}
