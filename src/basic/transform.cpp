#include "transform.h"
#include <rttr/registration>

using namespace rttr;
RTTR_REGISTRATION
{
    registration::class_<Transform>("Transform")
            .constructor<>()(rttr::policy::ctor::as_raw_ptr)
            .property("position", &Transform::get_position, &Transform::set_position)
            .property("rotation", &Transform::get_rotation, &Transform::set_rotation)
            .property("scale", &Transform::get_scale, &Transform::set_scale);
}

Transform::Transform():position_(0.f), scale_(1.f) {
    quaternion_ = glm::quat(1.f, 0.f, 0.f, 0.f);
}

