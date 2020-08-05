#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "uniforms.glsl"

// TODO(jan): Somehow bind this the the number of textures in the BSP.
layout(binding=1) uniform sampler2D atlas[59];

layout(binding=2) uniform samplerBuffer lightMap;

layout(location=0) in vec2 inTexCoord;
layout(location=1) in flat uint inTexIdx;
layout(location=2) in vec2 inLightCoord;
layout(location=3) in flat int inLightIdx;
layout(location=4) in flat vec2 inExtent;

layout(location=0) out vec4 outColor;

void main() {
    float frontAnimDelta = uniforms.elapsedS / 5.f;
    vec2 frontTexCoord = vec2(inTexCoord.x + frontAnimDelta, inTexCoord.y - frontAnimDelta);
    vec3 frontColor = texture(atlas[inTexIdx], frontTexCoord).rgb;
    float backAnimDelta = uniforms.elapsedS / 10.f;
    vec2 backTexCoord = vec2(inTexCoord.x + backAnimDelta, inTexCoord.y - backAnimDelta);
    vec3 backColor = texture(atlas[inTexIdx+1], backTexCoord).rgb;
    vec3 color = frontColor;
    if (frontColor.x + frontColor.y + frontColor.z < .01f) {
        color = backColor;
    }
    outColor = vec4(color, 1);
}
