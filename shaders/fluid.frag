#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "uniforms.glsl"

// TODO(jan): Somehow bind this the the number of textures in the BSP.
layout(binding=1) uniform sampler2D atlas[200];

layout(location=0) in flat uint inTexIdx;
layout(location=1) in vec3 inDir;

layout(location=0) out vec4 outColor;

void main() {
    vec3 dir = inDir;
    dir = normalize(dir) * 6*63 / 128.f;
    
    float scroll = uniforms.elapsedS / 8.f;
    vec2 texCoord = vec2(scroll + dir.x, scroll - dir.z);

    vec3 color = texture(atlas[inTexIdx], texCoord).rgb;
    outColor = vec4(color, 1);
}
