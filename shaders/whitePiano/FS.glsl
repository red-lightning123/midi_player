#version 330 core
out vec4 FragColor;

uniform int channel;
in vec2 texPos;

void main()
{
    vec4 color;
    int _channel = channel - 1;
	switch (_channel)
	{
    case -1: color = vec4(255, 255, 255, 255); break;
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
    FragColor = color;
}