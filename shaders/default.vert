#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location=0) in vec3 pos;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = vec4(pos, 1.f);
}
