#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "uniforms.glsl"

layout(location=0) in vec3 inPosition;
layout(location=1) in vec2 inTexCoord;
layout(location=2) in uint inTexIdx;
layout(location=3) in vec2 inLightCoord;
layout(location=4) in int inLightIdx;
layout(location=5) in vec2 inExtent;

layout(location=0) out flat uint outTexIdx;
layout(location=1) out vec3 dir;

void main() {
    gl_Position = uniforms.mvp * vec4(inPosition, 1.0);
    outTexIdx = inTexIdx;

    dir = inPosition - uniforms.origin;
}
