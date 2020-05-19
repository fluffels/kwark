#version 450
#extension GL_ARB_separate_shader_objects : enable

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
    float lightValue = 1.f;
    if (inLightIdx >= 0) {
        float s = inLightCoord.x;
        int sLeft = int(s);
        int sRight = sLeft + 1;
        float sLerp = s - sLeft;

        float t = inLightCoord.y;
        int tTop = int(t);
        int tBot = tTop + 1;
        float tLerp = t - tTop;

        int w = int(inExtent.x);

        int topLeftIdx = inLightIdx + sLeft + tTop * w;
        float topLeft = texelFetch(lightMap, topLeftIdx).r;

        int topRightIdx = inLightIdx + sRight + tTop * w;
        float topRight = texelFetch(lightMap, topRightIdx).r;

        float top = mix(topLeft, topRight, sLerp);

        int bottomLeftIdx = inLightIdx + sLeft + tBot * w;
        float bottomLeft = texelFetch(lightMap, bottomLeftIdx).r;

        int bottomRightIdx = inLightIdx + sRight + tBot * w;
        float bottomRight = texelFetch(lightMap, bottomRightIdx).r;

        float bottom = mix(bottomLeft, bottomRight, sLerp);

        lightValue = mix(top, bottom, tLerp);
    }

    vec3 texturedColor = texture(atlas[inTexIdx], inTexCoord).rgb;
    outColor = vec4(texturedColor * lightValue, 1);
}
