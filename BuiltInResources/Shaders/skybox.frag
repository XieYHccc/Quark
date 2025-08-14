#version 450

layout (set = 2, binding = 0) uniform samplerCube uCubeMap;

layout (location = 0) in vec3 inUVW;
layout (location = 0) out vec4 outFragColor;

void main() 
{
	outFragColor = texture(uCubeMap, inUVW);
}