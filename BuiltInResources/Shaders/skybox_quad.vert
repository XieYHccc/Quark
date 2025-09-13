#version 450
#extension GL_GOOGLE_include_directive : require
#include "include/camera_parameters.glslh"

layout (location = 0) in vec2 inPos;
layout (location = 0) out vec3 vDirection;

void main() 
{
    gl_Position = vec4(inPos, 1.0, 1.0);
    vDirection = (u_camera_parameters.inv_local_view_projection * vec4(inPos, 0.0, 1.0)).xyz;

}
