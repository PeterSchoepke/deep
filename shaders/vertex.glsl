#version 460

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_texcoord;
layout (location = 2) in vec3 a_normal;

layout (location = 0) out vec2 v_texcoord;
layout (location = 1) out vec3 v_normal;
layout (location = 2) out vec3 v_fragment_position;

layout(std140, set = 1, binding = 0) uniform UniformBlock {
    mat4 model;
    mat4 view;
    mat4 projection;
};

void main()
{
    gl_Position = projection * view * model * vec4(a_position, 1.0);
    v_texcoord = a_texcoord;
    v_normal = a_normal;
    v_fragment_position = vec3(model * vec4(a_position, 1.0));
}