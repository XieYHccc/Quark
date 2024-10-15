#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_ARB_gpu_shader_int64 : require
#include "include/uniform_scene.glslh"

layout (location = 0) out uvec2 o_id;

layout(push_constant, std430) uniform PushConstants
{
    layout(offset = 64) uint64_t entityID;
} u_pushConstant;

void main() 
{
    uint high = uint(u_pushConstant.entityID >> 32);
    uint low = uint(u_pushConstant.entityID & 0xFFFFFFFFu);
    o_id = uvec2(low, high);
}