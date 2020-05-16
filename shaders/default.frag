#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding=1) uniform sampler2D atlas[60];

layout(location=0) in vec2 inTexCoord;
layout(location=1) in flat uint inTexIdx;
layout(location=2) in vec3 inLight;

layout(location=0) out vec4 outColor;

void main() {
    vec3 texturedColor = texture(atlas[inTexIdx], inTexCoord).rgb;
    outColor = vec4(texturedColor, 1.0);
}
