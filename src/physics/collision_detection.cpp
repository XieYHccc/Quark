#include "./collision_detection.h"

#include <iostream>

#include "../physics/rigid_body_dynamics.h"
#include "../basic/object.h"
#include "../basic/transform.h"

glm::mat3 cross_matrix(const glm::vec3& vec) {
	glm::mat3 mat(0.f);
	mat[0][1] = vec[2];
	mat[0][2] = -vec[1];
	mat[1][0] = -vec[2];
	mat[1][2] = vec[0];
	mat[2][0] = vec[1];
	mat[2][1] = -vec[0];

	return mat;
}

// impluse method
bool check_collision(PlaneCollider* plane, MeshCollider* mesh) {
	// consider plane as a static rigid body, do not involve in dynamic update
	auto mesh_transform = dynamic_cast<Transform*>(mesh->get_object()->get_component("Transform"));
	auto mesh_rigid_body = dynamic_cast<RigidBodyDynamic*>(mesh->get_object()->get_component("RigidBodyDynamic"));

	// plane attributes
	glm::vec3 plane_normal = plane->normal;
	glm::vec3 plane_pos = plane->position;
	// mesh attributes
	glm::vec3 mesh_pos = mesh_transform->get_position();
	glm::mat3 rotation = glm::mat3_cast(mesh_transform->get_rotation());
	glm::vec3& v = mesh_rigid_body->velocity();
	glm::vec3& w = mesh_rigid_body->angular_velocity();
	float mass = mesh_rigid_body->mass();
	const glm::mat3 ref_inertia = mesh_rigid_body->reference_inertia();

	// 1.calculate collision position
	glm::vec3 collision_pos(0.f);
	int sum = 0;
	for (const auto& pos : mesh->trimesh->get_positions()) {
		glm::vec3 rotate_pos = rotation * pos;

		// determine if collision happened and velocity has been changed
		glm::vec3 xi = mesh_pos + rotate_pos;
		glm::vec3 vi = v + glm::cross(w, rotate_pos);
		if (glm::dot((xi - plane_pos), plane_normal) < 0.1 && glm::dot(vi, plane_normal) < 0) {
			collision_pos += rotate_pos;
			sum++;
		}

	}

	if (sum == 0) return false;

	if (mesh_rigid_body != nullptr) {
		collision_pos /= sum;	// average collision position
		glm::vec3 vi = v + glm::cross(w, collision_pos);

		// 2.calculate new wanted vi
		glm::vec3 vi_N = glm::dot(vi, plane_normal) * plane_normal;
		glm::vec3 vi_T = vi - vi_N;

		float uN = glm::length(vi_N) < 1 ? 0.01f : 0.2f; // bump coefficient
		float uT = 0.6; // friction coefficient
		float a = 1 - uT * (1 + uN) * glm::length(vi_N) / glm::length(vi_T);

		if (a < 0) a = 0;
		glm::vec3 vi_new = -uN * vi_N + a * vi_T;

		// 3.calculate impulse
		assert(mass > 0);
		glm::mat3 cross_mat = cross_matrix(collision_pos);
		glm::mat3 inv_inertia = glm::inverse(rotation * ref_inertia * glm::transpose(rotation));
		glm::mat3 K = (1 / mass) * glm::mat3(1.f) - cross_mat * inv_inertia * cross_mat;
		glm::vec3 impulse = glm::inverse(K) * (vi_new - vi);

		// 4.update v, w and position
		v = v + impulse / mass;
		w = w + inv_inertia * glm::cross(collision_pos, impulse);
		//mesh_transform->set_position(mesh_pos - glm::dot(collision_pos, plane_normal) * plane_normal);
	}

	return true;

}