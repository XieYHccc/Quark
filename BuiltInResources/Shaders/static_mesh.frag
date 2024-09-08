#version 450

#extension GL_GOOGLE_include_directive : require
#include "include/input_structures.glsl"

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;


//push constants block
layout( push_constant ) uniform PushConstants
{
	layout(offset = 72) float metalicFactor;
	layout(offset = 76) float roughnessFactor;
	layout(offset = 80) vec4 colorFactors;
} materialData;

void main() 
{
	float lightValue = max(dot(inNormal, sceneData.sunlightDirection.xyz), 0.1f);

	vec3 color = inColor * materialData.colorFactors.xyz * texture(colorTex,inUV).xyz;
	vec3 ambient = color *  sceneData.ambientColor.xyz;

	outFragColor = vec4(color * lightValue *  sceneData.sunlightColor.w + ambient ,1.0f);
}