#pragma once
#include <numeric>
#include <glm/glm.hpp>

namespace math {
class Aabb {
public:
    Aabb() : min_(std::numeric_limits<float>::max()), max_(-std::numeric_limits<float>::max()) {}
    Aabb(const glm::vec3& min, const glm::vec3& max) : min_(min), max_(max) {}

public:
    // Add point to the bounding box.
    Aabb& operator+=(const glm::vec3& p){
        for (int i = 0; i < 3; ++i) {
            if (p[i] < min_[i])
                min_[i] = p[i];
            if (p[i] > max_[i])
                max_[i] = p[i]; 
        }
        return *this;
    }
    
    // Add two bounding boxes.
    Aabb& operator+=(const Aabb& bb) {
        for (int i = 0; i < 3; ++i) {
            if (bb.min_[i] < min_[i])
                min_[i] = bb.min_[i];
            if (bb.max_[i] > max_[i])
                max_[i] = bb.max_[i];
        }
        return *this;
    }

    glm::vec3& min() { return min_; }
    glm::vec3& max() { return max_; }
    glm::vec3 get_center() const { return 0.5f * (min_ + max_); }
    glm::vec3 get_extents() const { return 0.5f * (max_ - min_); }
    glm::vec3 get_corner(uint32_t i) const;

    float get_radius() const;
    bool is_valid() const;
    Aabb transform(const glm::mat4& mat) const;

private:
    glm::vec3 min_, max_;
};

}