#version 450

#extension GL_GOOGLE_include_directive : require

#include "include/input_structures.glsl"

layout(location = 0) in vec3 inPosition;

#ifdef HAVE_NORMAL
layout(location = 1) in vec3 inNormal;
layout(location = 1) out vec3 vNormal;
#endif

#ifdef HAVE_UV
layout(location = 3) in vec2 inUV;
layout(location = 3) out vec2 vUV;
#endif

#ifdef HAVE_VERTEX_COLOR
layout(location = 4) in vec4 inColor;
layout(location = 4) out vec4 vColor;
#endif

//push constants block
layout(push_constant, std430) uniform PushConstants
{
	mat4 modelMatrix;
} modelData;

void main() 
{
	vec4 position = vec4(inPosition, 1.0f);
	gl_Position =  sceneData.viewproj * modelData.modelMatrix * position;

#ifdef HAVE_NORMAL
	mat3 normalTransform = mat3(modelData.modelMatrix[0].xyz, modelData.modelMatrix[1].xyz, modelData.modelMatrix[2].xyz);
	vNormal = normalize(normalTransform * inNormal);
#endif

#ifdef HAVE_UV
	vUV = inUV;
#endif

#ifdef HAVE_VERTEX_COLOR
	vColor = inColor;
#endif


}