#version 450

#extension GL_GOOGLE_include_directive : require
#include "include/input_structures.glsl"

layout(location = 0) in vec3 inPosition;

void main() 
{
	vec4 position = vec4(inPosition, 1.0f);
	gl_Position =  sceneData.viewproj * modelData.modelMatrix * position
}