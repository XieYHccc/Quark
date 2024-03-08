#pragma once

#include <memory>

#include <glm/glm.hpp>

#include "../Object/Components/component.h"

#include "../geometry/triangle_mesh.h"

class Object;
class RigidBodyDynamic : public Component {

public:
	RigidBodyDynamic(Object* object);

public:
	void init_velocity(const glm::vec3& v = glm::vec3(0.f), const glm::vec3& w = glm::vec3(0.f));

	void set_lauched(bool b) { launched_ = b; }

	float mass() { return mass_; }
	glm::vec3& velocity() { return v_; }
	glm::vec3& angular_velocity() { return w_; }
	const glm::mat3& reference_inertia() const { return inertia_ref_; }


public:
	void awake() override;
	void update() override;

private:
	bool launched_;
	
	// geometry attributes
	std::shared_ptr<TriMesh> trimesh_;

	// phsical material
	float mass_;
	glm::mat3 inertia_ref_; // reference 

	// physical state
	float dt_;
	float v_decay_;
	float w_decay_;
	glm::vec3 v_; // velocity
	glm::vec3 w_; // angular velocity
	
};