#version 330 core

layout(location = 0) in float h;
layout(location = 1) in float absoluteTime;

uniform mat4 M;
uniform mat4 VP;
uniform float timeElapsed;

out vec4 color;

void main()
{
    color = mix(vec4(147.0f, 81.0f, 255.0f, 255.0f), vec4(255.0f, 187.0f, 112.0f, 255.0f), abs(h));
    color /= 255.0f;
    mat4 MVP = VP * M;
    gl_Position = MVP * (vec4(128.0f * h, -10.0f, 0.1f * (absoluteTime - timeElapsed), 1.0f));
}
