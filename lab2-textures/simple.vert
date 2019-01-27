#version 420

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 texture;

out vec3 outColor;
out vec2 outTexture;

uniform mat4 projectionMatrix; 

// >>> @task 3.2

void main() 
{
	gl_Position = projectionMatrix * vec4(position, 1);
	outColor = color;
	outTexture = texture;

	// >>> @task 3.3
}