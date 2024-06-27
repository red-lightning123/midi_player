#version 330 core

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 texPos;

out vec2 fs_texPos;

uniform mat4 projection;

void main()
{
	fs_texPos = texPos;
	gl_Position = projection * vec4(pos, 0.0f, 1.0f);
}