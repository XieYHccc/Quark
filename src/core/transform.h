#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "component.h"

class Transform : public Component{
public:
    Transform();

    glm::vec3 get_position() const { return position_; }
    glm::quat get_rotation() const { return quaternion_; }
    glm::vec3 get_scale() const { return scale_; }

    void set_rotation(glm::quat quat) { quaternion_ = quat; }
    void set_rotation_by_angle(glm::vec3 euler_angle) { quaternion_ = glm::quat(euler_angle); } // angle is degree
    void set_position(glm::vec3 position) { position_ = position; }
    void set_scale(glm::vec3 scale) { scale_=scale; }

private:
    glm::quat quaternion_;
    glm::vec3 position_;
    glm::vec3 scale_;
};