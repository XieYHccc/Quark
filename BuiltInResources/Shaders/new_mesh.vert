#version 450

#extension GL_GOOGLE_include_directive : require
#include "include/camera_parameters.h"

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

#ifdef HAVE_BONE_INDEX
layout(location = 5) in uvec4 inJointIndices;
#endif

#ifdef HAVE_BONE_WEIGHT
layout(location = 6) in vec4 inJointWeights;
#endif

#if defined(HAVE_BONE_INDEX) && defined(HAVE_BONE_WEIGHT)
layout(std140, set = 3, binding = 1) uniform BonesWorld
{
    mat4 u_currentBoneWorldTransforms[256];
};
#else
struct StaticMeshInfo
{
    mat4 model;
};

layout(set = 2, binding = 0, std140) uniform PerVertexData
{
    StaticMeshInfo u_currentInfos[256];
};
#endif

void main() 
{
	vec4 position = vec4(inPosition, 1.0f);

	mat4 world_transform = u_currentInfos[gl_InstanceIndex].model;
	gl_Position = u_camera_parameters.view_projection * world_transform * position;

#ifdef HAVE_NORMAL
	mat3 normalTransform = mat3(world_transform[0].xyz, world_transform[1].xyz, world_transform[2].xyz);
	vNormal = normalize(normalTransform * inNormal);
#endif

#ifdef HAVE_UV
	vUV = inUV;
#endif

#ifdef HAVE_VERTEX_COLOR
	vColor = inColor;
#endif


}