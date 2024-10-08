#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_ARB_gpu_shader_int64 : require
#include "include/input_structures.glsl"

#ifdef HAVE_UV
layout(location = 1) in vec2 vUV;
#endif

#ifdef HAVE_NORMAL
layout(location = 2) in vec3 vNormal;
#endif

#ifdef HAVE_VERTEX_COLOR
layout(location = 3) in vec4 vColor;
#endif

layout (location = 0) out vec4 outColor;
layout (location = 1) out uvec2 outID;

layout(push_constant, std430) uniform PushConstants
{
	layout(offset = 64) vec4 baseColor;
	layout(offset = 80) float roughness;
	layout(offset = 84) float metallic;

    layout(offset = 88) uint64_t entityID;
} materialData;

void main() 
{
	vec3 baseColor = vec3(1.0, 1.0, 1.0);
	
#ifdef HAVE_UV
	baseColor *= (materialData.baseColor.xyz * texture(colorTex, vUV).xyz);
#endif

#ifdef HAVE_VERTEX_COLOR
    baseColor *= vColor.xyz;
#endif

#ifdef HAVE_NORMAL
	vec3 normal = normalize(vNormal);
#else
	const vec3 normal = vec3(0.0, 1.0, 0.0);
#endif

	float lightValue = max(dot(normal, sceneData.sunlightDirection.xyz), 0.1f);

	vec3 ambient = baseColor * sceneData.ambientColor.xyz;

	outColor = vec4(baseColor * lightValue * sceneData.sunlightColor.w + ambient ,1.0f);
	
    uint high = uint(materialData.entityID >> 32);
    uint low = uint(materialData.entityID & 0xFFFFFFFFu);
    outID = uvec2(low, high);

}