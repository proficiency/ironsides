#version 330 core

uniform mat4 projection;

in vec2  pos;
in vec2  in_uv;
out vec2 uv;

void main()
{
    gl_Position     = projection * vec4(pos, 0, 1);
    uv              = in_uv;
}
