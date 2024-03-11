#pragma once

#include <glm/glm.hpp>

#include "./collider.h"

#include "../Geometry/bounding_box.h"
class MeshCollider;
class PlaneCollider : public Collider {
public:
	using Collider::Collider;

	glm::vec3 normal;
	glm::vec3 position;

public:
	void awake() override;


	
};
