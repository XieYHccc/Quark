#ifndef RENDER_PARAMETERS_H
#define RENDER_PARAMETERS_H

layout(set = 0, binding = 0, std140) uniform CameraParameters
{
	mat4 projection;
	mat4 view;
	mat4 view_projection;
	mat4 inv_projection;
	mat4 inv_view;
	mat4 inv_view_projection;

	vec3 camera_position;
	vec3 camera_front;
	vec3 camera_right;
	vec3 camera_up;

	float z_near;
	float z_far;
} u_camera_parameters;

float clip_z_to_linear(float clip_z)
{
	vec2 z = u_camera_parameters.inv_projection[2].zw * clip_z + u_camera_parameters.inv_projection[3].zw;
	return -z.x / z.y;
}

#endif