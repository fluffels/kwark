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
layout(location=1) out vec2 outTexCoordFront;
layout(location=2) out vec2 outTexCoordBack;

void main() {
    gl_Position = uniforms.mvp * vec4(inPosition, 1.0);
    outTexIdx = inTexIdx;

    vec3 dir = inPosition - uniforms.origin;
    dir.y *= 3;
    float length = dot(dir, dir);
    length = sqrt(length);
    length = 6*63/length;

    float scroll = uniforms.elapsedS * 100 / 8.f;
    outTexCoordFront = vec2(
        (scroll + dir.x * length) * (1.f/128),
        (scroll + dir.z * length) * (1.f/128)
    );

    scroll = scroll / 2.f;
    outTexCoordBack = vec2(
        (scroll + dir.x * length) * (1.f/128),
        (scroll + dir.z * length) * (1.f/128)
    );
}
