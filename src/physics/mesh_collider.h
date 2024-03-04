#pragma once

#include <memory>

#include <glm/glm.hpp>

#include "./collider.h"
#include "../geometry/bounding_box.h"

class TriMesh;
class PlaneCollider;
class MeshCollider : public Collider {
public:
	using Collider::Collider;

public:
	void set_trimesh(std::shared_ptr<TriMesh> trimesh) { trimesh_ = trimesh; }

public:
	void awake() override;

private:
	std::shared_ptr<TriMesh> trimesh_;

	friend bool check_collision(PlaneCollider& plane, MeshCollider& mesh);
};