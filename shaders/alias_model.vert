#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "uniforms.glsl"

layout(push_constant) uniform PushConstants {
    vec3 translate;
    float angle;
} pushConstants;

layout(location=0) in vec3 inPosition;
layout(location=1) in vec2 inTexCoord;

layout(location=0) out vec2 outTexCoord;

void main() {
    float theta = pushConstants.angle * 3.14 / 180;
    float cs = cos(theta);
    float sn = sin(theta);
    mat4 rotation = mat4(1.0);
    rotation[0][0] = cs;
    rotation[0][2] = -sn;
    rotation[2][0] = sn;
    rotation[2][2] = cs;
    vec4 position = vec4(pushConstants.translate, 0) + (rotation * vec4(inPosition, 1.0));
    gl_Position = uniforms.mvp * position;
    outTexCoord = inTexCoord;
}
