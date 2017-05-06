#version 330
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 offset;

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;
out vec3 fColor;


void main()
{
	gl_Position = P * V * M * vec4( position + offset, 1.0f );
	fColor = color;
}
