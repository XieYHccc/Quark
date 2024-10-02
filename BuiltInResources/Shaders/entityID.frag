#version 450

layout(location = 0) int64_t outId;

layout(push_constant, std430) uniform PushConstants
{
	int64_t entityId;
} modelData

void main() 
{
    outId = entityId
}