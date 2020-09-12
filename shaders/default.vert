#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "uniforms.glsl"

layout(location=0) in vec3 inPosition;
layout(location=1) in vec2 inTexCoord;
layout(location=2) in uint inTexIdx;
layout(location=3) in vec2 inLightCoord;
layout(location=4) in int inLightIdx;
layout(location=5) in int inLightStyle1;
layout(location=6) in int inLightStyle2;
layout(location=7) in int inLightStyle3;
layout(location=8) in int inLightStyle4;
layout(location=9) in vec2 inExtent;

layout(location=0) out vec2 outTexCoord;
layout(location=1) out flat uint outTexIdx;
layout(location=2) out vec2 outLightCoord;
layout(location=3) out flat int outLightIdx;
layout(location=4) out float outLight1;
layout(location=5) out float outLight2;
layout(location=6) out float outLight3;
layout(location=7) out float outLight4;
layout(location=8) out flat vec2 outExtent;

void main() {
    gl_Position = uniforms.mvp * vec4(inPosition, 1.0);
    outTexIdx = inTexIdx;
    outTexCoord = inTexCoord;
    outLightCoord = inLightCoord;
    outLightIdx = inLightIdx;
    if (inLightStyle1 > 11) {
        outLight1 = 0;
    } else {
        outLight1 = uniforms.light[inLightStyle1];
    }
    if (inLightStyle2 > 11) {
        outLight2 = 0;
    } else {
        outLight2 = uniforms.light[inLightStyle2];
    }
    if (inLightStyle3 > 11) {
        outLight3 = 0;
    } else {
        outLight3 = uniforms.light[inLightStyle3];
    }
    if (inLightStyle4 > 11) {
        outLight4 = 0;
    } else {
        outLight4 = uniforms.light[inLightStyle4];
    }
    outExtent = inExtent;
}
