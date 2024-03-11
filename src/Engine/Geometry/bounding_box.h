#pragma once

#include <numeric>

#include <glm/glm.hpp>

class BoundingBox {
public:

    BoundingBox() : min_(std::numeric_limits<float>::max()), max_(-std::numeric_limits<float>::max()) {}

    BoundingBox(const glm::vec3& min, const glm::vec3& max) : min_(min), max_(max) {}

public:
    // Add point to the bounding box.
    BoundingBox& operator+=(const glm::vec3& p){
        for (int i = 0; i < 3; ++i) {
            if (p[i] < min_[i])
                min_[i] = p[i];
            if (p[i] > max_[i])
                max_[i] = p[i]; 
        }
        return *this;
    }
    // Add two bounding boxes.
    BoundingBox& operator+=(const BoundingBox& bb) {
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
    glm::vec3 center() const { return 0.5f * (min_ + max_); }

    bool is_valid() const { return (max_[0] > min_[0] && max_[1] > min_[1] && max_[2] > min_[2]);}

private:
    glm::vec3 min_, max_;
};
