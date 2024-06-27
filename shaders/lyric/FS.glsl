#version 330 core
out vec4 FragColor;

in vec2 fs_texPos;

uniform sampler2D glyphTexture;

void main()
{
	FragColor = vec4(texture(glyphTexture, fs_texPos).r);
}