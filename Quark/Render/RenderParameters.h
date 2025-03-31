#pragma once
#include <glm/glm.hpp>

namespace quark 
{
struct CameraParameters
{
	glm::mat4 projection;
	glm::mat4 view;
	glm::mat4 view_projection;
	glm::mat4 inv_projection;
	glm::mat4 inv_view;
	glm::mat4 inv_view_projection;
	// glm::mat4 local_view_projection;
	// glm::mat4 inv_local_view_projection;

	// glm::mat4 unjittered_view_projection;
	// glm::mat4 unjittered_inv_view_projection;
	// glm::mat4 unjittered_prev_view_projection;

	// glm::mat4 multiview_view_projection[NumShadowCascades];

	alignas(16) glm::vec3 camera_position;
	alignas(16) glm::vec3 camera_front;
	alignas(16) glm::vec3 camera_right;
	alignas(16) glm::vec3 camera_up;

	float z_near;
	float z_far;
};

struct DirectionalParameters
{
	alignas(16) glm::vec3 color;
	alignas(16) glm::vec3 direction;
};

struct LightingParameters
{
    DirectionalParameters directional;
};

}