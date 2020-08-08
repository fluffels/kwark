#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "uniforms.glsl"

// TODO(jan): Somehow bind this the the number of textures in the BSP.
layout(binding=1) uniform sampler2D atlas[2];

layout(location=0) in flat uint inTexIdx;
layout(location=1) in vec3 inDir;

layout(location=0) out vec4 outColor;

void main() {
    vec3 dir = inDir;
    dir.y *= 3;
    dir = normalize(dir) * 6*63 / 128.f;
    float scroll = uniforms.elapsedS / 8.f;
    vec2 texCoordFront = vec2(scroll + dir.x, scroll - dir.z);

    scroll = scroll / 2.f;
    vec2 texCoordBack = vec2(scroll + dir.x, scroll - dir.z);

    vec3 frontColor = texture(atlas[inTexIdx], texCoordFront).rgb;
    vec3 backColor = texture(atlas[inTexIdx+1], texCoordBack).rgb;
    vec3 color = frontColor;
    if (frontColor.x + frontColor.y + frontColor.z < .01f) {
        color = backColor;
    }
    outColor = vec4(color, 1);
}
