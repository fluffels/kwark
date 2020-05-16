#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding=1) uniform sampler2D atlas[60];

layout(location=0) in float inDistance;
layout(location=1) in vec2 inTexCoord;
layout(location=2) in flat uint inTexIdx;
layout(location=3) in vec3 inLight;

layout(location=0) out vec4 outColor;

const vec3 BACKGROUND = vec3(1.f, 1.f, 1.f);
const float MAX_D = 1000.f;

void main() {
    vec3 texturedColor = texture(atlas[inTexIdx], inTexCoord).rgb;
    outColor = vec4(texturedColor, 1.0);
}
