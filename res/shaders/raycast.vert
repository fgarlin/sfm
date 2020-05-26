#version 450 core

layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_texCoord;

uniform mat4 mvp;

out vec3 v_texCoord;

void main()
{
    gl_Position = mvp * vec4(a_pos, 1.0);
    v_texCoord = a_texCoord;
}
