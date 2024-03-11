#pragma once

#include <memory>

#include <glm/glm.hpp>

#include "./collider.h"
#include "../Geometry/bounding_box.h"

class TriMesh;
class PlaneCollider;
class MeshCollider : public Collider {
public:
	using Collider::Collider;

	std::shared_ptr<TriMesh> trimesh;

public:
	void set_trimesh(std::shared_ptr<TriMesh> trimesh) { trimesh = trimesh; }

public:
	void awake() override;


};