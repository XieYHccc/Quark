#include "./rigid_body_dynamics.h"

#include <iostream>

#include <rttr/registration.h>

#include "../basic/object.h"
#include "../basic/transform.h"
#include "../render/components/mesh_filter.h"

using namespace rttr;
RTTR_REGISTRATION
{
	registration::class_<RigidBodyDynamic>("RigidBodyDynamic")
			.constructor<>()(rttr::policy::ctor::as_raw_ptr);
}

RigidBodyDynamic::RigidBodyDynamic() {
	inertia_ref_ = glm::mat3(0.f);
	init();
}

void RigidBodyDynamic::init() {
	launched_ = false;
	dt_ = 0.005f;
	v_decay_ = 0.999f;
	w_decay_ = 0.98f;
	v_ = glm::vec3(0.f, 0.f, 0.f);
	w_ = glm::vec3(0.f, 0.f, 0.f);
}
void RigidBodyDynamic::awake() {
	init();

	auto mesh_fileter = dynamic_cast<MeshFilter*>(get_object()->get_component("MeshFilter"));
	if (mesh_fileter == nullptr) {
		std::cerr << "RigidBodyDynamic::awake(): object doesn't have mesh_fileter component.";
		return;
	}

	 trimesh_ = TriMesh(mesh_fileter->mesh());
	// calculate mass and reference inertia matrix
	for (auto& p : trimesh_.get_positions()) {
		// assume the mass of each vertex is 1
		mass_++;

		glm::mat3 mat = (glm::dot(p, p) * glm::mat3(1.f)) - glm::outerProduct(p, p);
		inertia_ref_ += mat;
	}
}

void RigidBodyDynamic::update() {
	if (!launched_)
		return;

	auto transform = dynamic_cast<Transform*>(get_object()->get_component("Transform"));
	glm::vec3 position = transform->get_position();
	glm::quat quat = transform->get_rotation();

	// update velocity and position
	glm::vec3 force_per_point = glm::vec3(0.f, -10.f, 0.f);
	glm::vec3 force = mass_ * force_per_point;
	v_ = v_decay_ * (v_ + (dt_ * force / mass_));
	transform->set_position(position + v_ * dt_);


	//glm::vec3 angle = glm::eulerAngles(quat);
	//transform->set_rotation_by_angle(angle + glm::vec3(0.1f, 0.f, 0.f));

	//update angular velocity and quaternion
	glm::vec3 torque(0.f);
	glm::mat3 rotation = glm::mat3_cast(quat);
	glm::mat3 inertia = rotation * inertia_ref_ * glm::transpose(rotation);
	for (const auto& pos : trimesh_.get_positions()) {
		auto tmp = glm::cross((rotation * pos), force_per_point);
		torque += glm::cross((rotation * pos), force_per_point);
	} 
	w_ = w_ + dt_ * glm::inverse(inertia) * torque;
	w_ = w_decay_ * w_;
	quat += glm::quat(0, 0.5f * dt_ * w_) * quat;
	glm::normalize(quat);
	transform->set_rotation(quat);

};