#pragma once

#include <glm/glm.hpp>

#include "../core/component.h"

#include "../geometry/triangle_mesh.h"

class RigidBodyDynamic : public Component {

public:
	RigidBodyDynamic();

public:
	void init();
	void set_velocity(glm::vec3 v) { v_ = v; }
	void set_angular_velocity(glm::vec3 w) { w_ = w; }
	void set_lauched(bool b) { launched_ = b; }

public:
	void awake() override;
	void update() override;

private:
	bool launched_;

	// geometry
	TriMesh trimesh_;
	
	// physics state
	float mass_;
	float dt_;
	float v_decay_;
	float w_decay_;
	glm::vec3 v_; // velocity
	glm::vec3 w_; // angular velocity
	glm::mat3 inertia_ref_; // reference inertia
	
};