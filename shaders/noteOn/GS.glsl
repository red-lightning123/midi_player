#version 330 core

layout(points) in;
layout(triangle_strip, max_vertices = 6) out;

uniform mat4 M;
uniform mat4 VP;

out vec2 texPos;

uniform vec3 cpos = vec3(0.0f);

void main()
{
	mat4 MVP = VP * M;
	vec4 v1 = MVP * vec4(cpos + vec3(-1.0f, -1.0f, 0.0f), 1.0f);
	vec4 v2 = MVP * vec4(cpos + vec3(-1.0f, 1.0f, 0.0f), 1.0f);
	vec4 v3 = MVP * vec4(cpos + vec3(1.0f, -1.0f, 0.0f), 1.0f);
	vec4 v4 = MVP * vec4(cpos + vec3(1.0f, 1.0f, 0.0f), 1.0f);

	gl_Position = v1;
	texPos = vec2(0.0f, 0.0f);
	EmitVertex();
	gl_Position = v2;
	texPos = vec2(0.0f, 1.0f);
	EmitVertex();
	gl_Position = v3;
	texPos = vec2(1.0f, 0.0f);
	EmitVertex();
	EndPrimitive();

	gl_Position = v4;
	texPos = vec2(1.0f, 1.0f);
	EmitVertex();
	gl_Position = v2;
	texPos = vec2(0.0f, 1.0f);
	EmitVertex();
	gl_Position = v3;
	texPos = vec2(1.0f, 0.0f);
	EmitVertex();
	EndPrimitive();
}