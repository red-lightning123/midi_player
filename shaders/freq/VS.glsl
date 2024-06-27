#version 330 core

layout(location = 0) in float f;
layout(location = 1) in float mag;
layout(location = 2) in float absoluteTime;

uniform mat4 M;
uniform mat4 VP;
uniform float timeElapsed;

out vec4 color;

void main()
{
    const float scale = 0.1f;
    const float colorScale = 1.0f * scale;
    color = mix(vec4(147.0f, 81.0f, 255.0f, 255.0f), vec4(255.0f, 187.0f, 112.0f, 255.0f), mag * colorScale);
    color /= 255.0f;
    mat4 MVP = VP * M;
    const float lowestNote = 8.18f;
    const float highestNote = 12543.85f;
    gl_Position = MVP * (vec4(256.0f * (log(f) - log(lowestNote)) / (log(highestNote) - log(lowestNote)) - 128.0f, scale * mag, 0.1f * (absoluteTime - timeElapsed), 1.0f));
}
