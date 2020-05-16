#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding=1) uniform sampler2D atlas;

layout(location=0) in float inDistance;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec3 inLight;

layout(location=0) out vec4 outColor;

const vec3 BACKGROUND = vec3(1.f, 1.f, 1.f);
const float MAX_D = 1000.f;

void main() {
    // TODO(jan): replace with actual texture color
    vec2 texCoord = vec2(.5f, .5f);
    vec3 texturedColor = texture(atlas, texCoord).rgb;
    // vec3 texturedColor = inNormal;
    vec3 lightedColor = texturedColor * (inLight + .5f);
    float fogCoefficient = inDistance / MAX_D;
    vec3 foggedColor = mix(lightedColor, BACKGROUND, fogCoefficient);
    outColor = vec4(foggedColor, 1.0);
}
