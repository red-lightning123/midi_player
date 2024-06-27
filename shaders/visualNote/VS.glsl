#version 330 core

layout(location = 0) in uint absoluteTime;
layout(location = 1) in uint key;
layout(location = 2) in uint channel;

uniform mat4 M;
uniform mat4 VP;
uniform float timeElapsed;

out vec4 color;

void main()
{
    switch (int(channel))
    {
    case 0: color = vec4(241, 70, 57, 255); break;
    case 1: color = vec4(205, 241, 0, 255); break;
    case 2: color = vec4(50, 201, 20, 255); break;
    case 3: color = vec4(107, 241, 231, 255); break;
    case 4: color = vec4(127, 67, 255, 255); break;
    case 5: color = vec4(241, 127, 200, 255); break;
    case 6: color = vec4(170, 212, 170, 255); break;
    case 7: color = vec4(222, 202, 170, 255); break;
    case 8: color = vec4(241, 201, 20, 255); break;
    case 9: color = vec4(80, 80, 80, 255); break;
    case 10: color = vec4(202, 50, 127, 255); break;
    case 11: color = vec4(0, 132, 255, 255); break;
    case 12: color = vec4(102, 127, 37, 255); break;
    case 13: color = vec4(241, 164, 80, 255); break;
    case 14: color = vec4(107, 30, 107, 255); break;
    case 15: color = vec4(50, 127, 127, 255); break;
    }
    color /= 255.0f;
    if (gl_VertexID % 2 == 1)
        color = vec4(color.rgb, 0.2f);
    mat4 MVP = VP * M;
    gl_Position = MVP * (vec4(2.0f * (int(key) - 64), 2.0f * channel, 0.1f * (float(absoluteTime) - timeElapsed), 1.0f));
}
