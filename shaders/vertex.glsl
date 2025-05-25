#version 460

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_texcoord;

layout (location = 0) out vec2 v_texcoord;

layout(std140, set = 1, binding = 0) uniform UniformBlock {
    mat4 transform;
};

void main()
{
    gl_Position = transform * vec4(a_position, 1.0f);
    v_texcoord = a_texcoord;
}