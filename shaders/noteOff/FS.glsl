#version 330 core
out vec4 FragColor;

uniform sampler2D tex;
in vec2 texPos;

void main()
{
    FragColor = texture(tex, texPos) / 4.0f;
}