#version 460

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_texcoord;

layout (location = 0) out vec2 v_texcoord;

layout(std140, set = 1, binding = 0) uniform UniformBlock {
    mat4 model;
    mat4 view;
    mat4 projection;
};

void main()
{
    gl_Position = projection * view * model * vec4(a_position, 1.0);
    v_texcoord = a_texcoord;
}