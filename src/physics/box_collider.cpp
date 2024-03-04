#include "./box_collider.h"

#include <rttr/registration.h>

#include "../basic/object.h"
#include "../render/components/mesh_filter.h"

using namespace rttr;
RTTR_REGISTRATION
{
	registration::class_<PlaneCollider>("PlaneCollider")
			.constructor<>()(rttr::policy::ctor::as_raw_ptr);
}

void PlaneCollider::awake() {
	auto mesh_filter = dynamic_cast<MeshFilter*>(get_object()->get_component("MeshFilter"));
	auto trimesh = mesh_filter->trimesh();
	if (!mesh_filter || !trimesh)
		return;

	auto tri = trimesh->get_triangles()[0];
	const glm::vec3& p1 = trimesh->get_positions()[tri.indices[0]];
	const glm::vec3& p2 = trimesh->get_positions()[tri.indices[1]];
	const glm::vec3& p3 = trimesh->get_positions()[tri.indices[2]];
	const glm::vec3& v1 = p2 - p1;
	const glm::vec3& v2 = p3 - p1;

	normal_ = glm::cross(v1, v2);
	glm::normalize(normal_);
	position_ = p1;
}