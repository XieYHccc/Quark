#pragma once
#include <numeric>
#include <glm/glm.hpp>

namespace quark::math {
class Aabb {
public:
    Aabb() : min_(std::numeric_limits<float>::max()), max_(-std::numeric_limits<float>::max()) {}
    Aabb(const glm::vec3& min, const glm::vec3& max) : min_(min), max_(max) {}

public:
    // Add point to the bounding box.
    Aabb& operator+=(const glm::vec3& p);
    // Add two bounding boxes.
    Aabb& operator+=(const Aabb& bb);

    glm::vec3 Min() { return min_; }
    glm::vec3 Max() { return max_; }
    glm::vec3 GetCenter() const { return 0.5f * (min_ + max_); }
    glm::vec3 GetExtents() const { return 0.5f * (max_ - min_); }
    glm::vec3 GetCorner(uint32_t i) const;

    float GetRadius() const;
    bool IsValid() const;
    Aabb Transform(const glm::mat4& mat) const;

private:
    glm::vec3 min_, max_;
};

}