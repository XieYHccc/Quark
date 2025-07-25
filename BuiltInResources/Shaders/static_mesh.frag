#version 450

#extension GL_GOOGLE_include_directive : require
#include "include/camera_parameters.glslh"
#include "include/lighting_parameters.glslh"

#ifdef HAVE_NORMAL
layout(location = 1) in vec3 vNormal;
#endif

#ifdef HAVE_UV
layout(location = 3) in vec2 vUV;
#endif

#ifdef HAVE_VERTEX_COLOR
layout(location = 4) in vec4 vColor;
#endif

layout (location = 0) out vec4 outFragColor;

layout(set = 1, binding = 0) uniform sampler2D u_albedoTex;
// layout(set = 2, binding = 1) uniform sampler2D u_normalTex;
// layout(set = 2, binding = 2) uniform sampler2D u_metalRoughTex;

layout(std430, push_constant) uniform StaticMeshFragment
{
    vec4 base_color;
    vec4 emissive;
    float roughness;
    float metallic;
    float normal_scale;
} u_staticmesh_frag;

void main() 
{
	vec3 baseColor = vec3(1.0, 1.0, 1.0);
	
	vec3 directional_light_dir = u_lighting_parameters.directional.direction;
	vec3 directional_light_color = u_lighting_parameters.directional.color;
	directional_light_dir = normalize(directional_light_dir);

#ifdef HAVE_UV
	baseColor *= (u_staticmesh_frag.base_color.xyz * texture(u_albedoTex, vUV).xyz);
#endif

#ifdef HAVE_VERTEX_COLOR
    baseColor *= vColor.xyz;
#endif

#ifdef HAVE_NORMAL
	vec3 normal = normalize(vNormal);
#else
	const vec3 normal = vec3(0.0, 1.0, 0.0);
#endif

	// lighting calculations
	float ldotn = max(dot(normal, directional_light_dir), 0.1f);
	vec3 diffuse = baseColor * directional_light_color * ldotn;
	vec3 ambient = baseColor * 0.1;
	vec3 result = diffuse + ambient;

	outFragColor = vec4(result ,1.0f);
}