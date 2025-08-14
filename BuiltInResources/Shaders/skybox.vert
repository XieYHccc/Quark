#version 450
#extension GL_GOOGLE_include_directive : require
#include "include/camera_parameters.glslh"

layout (location = 0) in vec3 inPos;
layout (location = 0) out vec3 outUVW;

void main() 
{
	outUVW = inPos;

	// Convert cubemap coordinates into Vulkan coordinate space
	// outUVW.xy *= -1.0;
	
	// Remove translation from view matrix
	mat4 viewMat = mat4(mat3(u_camera_parameters.view));
	vec4 pos = u_camera_parameters.projection * viewMat * vec4(inPos.xyz, 1.0);
	gl_Position = pos.xyww;
}
