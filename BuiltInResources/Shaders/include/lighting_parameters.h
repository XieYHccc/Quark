#ifndef RENDER_PARAMETERS_H
#define RENDER_PARAMETERS_H

struct DirectionalParameters
{
	vec3 color;
	vec3 direction;
};

layout(set = 0, binding = 1, std140) uniform LightingParameters
{
	DirectionalParameters directional;
} u_lighting_parameters;

#endif