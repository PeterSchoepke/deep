#version 460

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_texcoord;
layout (location = 0) out vec4 v_color;

void main()
{
    gl_Position = vec4(a_position, 1.0f);
    v_color = vec4(a_texcoord, 0.0, 1.0);
}