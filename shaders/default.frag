#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in vec3 inColor;
layout(location=1) in float inDistance;

layout(location=0) out vec4 outColor;

const vec3 BACKGROUND = vec3(1.f, 1.f, 1.f);
const float MAX_D = 1000.f;

void main() {
    if (inDistance > MAX_D) {
        discard;
    }
    float fade = inDistance / MAX_D;
    vec3 mixColor = mix(inColor, BACKGROUND, fade);
    outColor = vec4(mixColor, 1.0);
}
