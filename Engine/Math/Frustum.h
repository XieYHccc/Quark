#pragma once
#include <glm/glm.hpp>
#include "Math/Aabb.h"

namespace math {
class Frustum {
public:
	enum side { LEFT = 0, RIGHT = 1, NEAR = 2, FAR = 3, TOP = 4, BOTTOM = 5 };
	
    Frustum() = default;

    void build(const glm::mat4& inv_view_proj_mat);
    bool check_shpere(const Aabb& aabb);
private:
    glm::mat4 inv_view_proj_matrix_;
    std::array<glm::vec4, 6> planes;
};

}