#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "uniforms.glsl"

// TODO(jan): Somehow bind this the the number of textures in the BSP.
layout(binding=1) uniform sampler2D atlas[200];

layout(location=0) in vec2 inTexCoord;
layout(location=1) in flat uint inTexIdx;
layout(location=2) in vec2 inLightCoord;
layout(location=3) in flat int inLightIdx;
layout(location=4) in flat vec2 inExtent;

layout(location=0) out vec4 outColor;

void main() {
    vec2 texCoord = inTexCoord;

    float amplitude = 0.1;
    float speed = 1;

    float offset = uniforms.elapsedS * speed;

    float x = texCoord.x;
    float y = texCoord.y;

    texCoord.x = x + amplitude * sin(offset + y);
    texCoord.y = y + amplitude * sin(offset + x);

    vec3 color = texture(atlas[inTexIdx], texCoord).rgb;
    outColor = vec4(color, 1);
}
