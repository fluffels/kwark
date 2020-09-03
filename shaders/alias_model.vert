#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "uniforms.glsl"

layout(push_constant) uniform pushConstants {
    vec3 translate;
} PushConstants;

layout(location=0) in vec3 inPosition;

void main() {
    gl_Position = uniforms.mvp * vec4(inPosition, 1.0);
}
