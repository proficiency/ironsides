#version 330 core

uniform mat4 u_projection;

in vec2  in_pos;
in vec2  in_uv;

out vec2 uv;

void main()
{
    gl_Position = u_projection * vec4(in_pos, 0, 1);
    uv          = in_uv;
}
