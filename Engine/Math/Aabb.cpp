#include "Math/Aabb.h"

namespace math {

glm::vec3 Aabb::get_corner(uint32_t i) const
{
    float x = i & 1 ? max_.x : min_.x;
    float y = i & 2 ? max_.y : min_.y;
    float z = i & 4 ? max_.z : min_.z;
    return glm::vec3(x, y, z);
}

float Aabb::get_radius() const
{
    return 0.5f * glm::length(max_ - min_);
}

bool Aabb::is_valid() const 
{
    return (max_[0] > min_[0] && max_[1] > min_[1] && max_[2] > min_[2]);
}

Aabb Aabb::transform(const glm::mat4 &mat) const 
{
	glm::vec3 min = glm::vec3(FLT_MAX);
	glm::vec3 max = glm::vec3(-FLT_MAX);

	for (size_t i = 0; i < 8; i++)
	{
		glm::vec3 corner = get_corner(i);
		glm::vec4 trans = mat * glm::vec4(corner, 1.0f);
		glm::vec3 v = trans.xyz();
		min = glm::min(v, min);
		max = glm::max(v, max);
	}

	return Aabb(min, max);
}
}