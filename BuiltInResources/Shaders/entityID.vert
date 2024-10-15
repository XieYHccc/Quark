#version 450

#extension GL_GOOGLE_include_directive : require

#include "include/uniform_scene.glslh"

layout(location = 0) in vec3 inPosition;

//push constants block
layout(push_constant, std430) uniform PushConstants
{
	mat4 modelMatrix;
} u_modelData;

void main() 
{
	vec4 position = vec4(inPosition, 1.0f);
	gl_Position =  sceneData.viewproj * u_modelData.modelMatrix * position;
}