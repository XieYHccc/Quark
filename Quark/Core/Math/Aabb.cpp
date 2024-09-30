#include "Quark/qkpch.h"
#include "Quark/Core/Math/Aabb.h"

namespace quark::math {

glm::vec3 Aabb::GetCorner(uint32_t i) const
{
    float x = i & 1 ? max_.x : min_.x;
    float y = i & 2 ? max_.y : min_.y;
    float z = i & 4 ? max_.z : min_.z;
    return glm::vec3(x, y, z);
}

float Aabb::GetRadius() const
{
    return 0.5f * glm::length(max_ - min_);
}

bool Aabb::IsValid() const 
{
    return (max_[0] >= min_[0] && max_[1] >= min_[1] && max_[2] >= min_[2]);
}

Aabb Aabb::Transform(const glm::mat4 &mat) const 
{
	glm::vec3 min = glm::vec3(FLT_MAX);
	glm::vec3 max = glm::vec3(-FLT_MAX);

	for (uint32_t i = 0; i < 8; i++)
	{
		glm::vec3 corner = GetCorner(i);
		glm::vec4 trans = mat * glm::vec4(corner, 1.0f);
		glm::vec3 v = trans.xyz();
		min = glm::min(v, min);
		max = glm::max(v, max);
	}

	return Aabb(min, max);
}

Aabb& Aabb::operator+=(const glm::vec3& p)
{
	min_ = glm::min(min_, p);
	max_ = glm::max(max_, p);
	return *this;
}

Aabb& Aabb::operator+=(const Aabb& bb) {
	for (int i = 0; i < 3; ++i) {
		if (bb.min_[i] < min_[i])
			min_[i] = bb.min_[i];
		if (bb.max_[i] > max_[i])
			max_[i] = bb.max_[i];
	}
	return *this;
}

}