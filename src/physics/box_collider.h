#pragma once

#include <glm/glm.hpp>

#include "./collider.h"

#include "../geometry/bounding_box.h"
class MeshCollider;
class PlaneCollider : public Collider {
public:
	using Collider::Collider;

public:
	void awake() override;

public:
	glm::vec3 normal_;
	glm::vec3 position_;

	friend bool check_collision(PlaneCollider& plane, MeshCollider& mesh);
	
};
