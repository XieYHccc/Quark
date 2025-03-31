#pragma once
#include <array>
#include <glm/glm.hpp>

#include "Quark/Core/Math/Aabb.h"

namespace quark::math {
class Frustum {
public:
	enum side { LEFT = 0, RIGHT = 1, NEAR = 2, FAR = 3, TOP = 4, BOTTOM = 5 };
	
    Frustum() = default;
    Frustum(const glm::mat4& inv_view_proj_mat);

    void Build(const glm::mat4& inv_view_proj_mat);
    bool CheckSphere(const Aabb& aabb) const;
private:
    glm::mat4 inv_view_proj_matrix_;
    std::array<glm::vec4, 6> planes;
};

}