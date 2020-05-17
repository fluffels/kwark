#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform Push {
    vec3 color;
} push;

layout(binding=0) uniform Uniform {
    mat4x4 mvp;
} uniforms;

layout(location=0) in vec3 inPosition;
layout(location=1) in vec2 inTexCoord;
layout(location=2) in uint inTexIdx;
layout(location=3) in vec2 inLightCoord;
layout(location=4) in int inLightIdx;

layout(location=0) out vec2 outTexCoord;
layout(location=1) out flat uint outTexIdx;
layout(location=2) out vec2 outLightCoord;
layout(location=3) out flat int outLightIdx;

void main() {
    gl_Position = uniforms.mvp * vec4(inPosition, 1.0);
    outTexIdx = inTexIdx;
    outTexCoord = inTexCoord;
    outLightCoord = inLightCoord;
    outLightIdx = inLightIdx;
}
