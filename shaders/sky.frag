#version 450
#extension GL_ARB_separate_shader_objects : enable

// TODO(jan): Somehow bind this the the number of textures in the BSP.
layout(binding=1) uniform sampler2D atlas[2];

layout(location=0) in flat uint inTexIdx;
layout(location=1) in vec2 inTexCoordFront;
layout(location=2) in vec2 inTexCoordBack;

layout(location=0) out vec4 outColor;

void main() {
    vec3 frontColor = texture(atlas[inTexIdx], inTexCoordFront).rgb;
    vec3 backColor = texture(atlas[inTexIdx+1], inTexCoordBack).rgb;
    vec3 color = frontColor;
    if (frontColor.x + frontColor.y + frontColor.z < .01f) {
        color = backColor;
    }
    outColor = vec4(color, 1);
}
