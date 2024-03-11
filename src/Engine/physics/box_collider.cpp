#include "./box_collider.h"

#include "../Object/Object.h"
#include "../Object/Components/MeshFilterCmpt/MeshFilterCmpt.h"

void PlaneCollider::awake() {
	auto mesh_filter = get_object()->get_component<MeshFilterCmpt>();
	auto trimesh = mesh_filter->trimesh();
	if (!mesh_filter || !trimesh)
		return;

	auto tri = trimesh->get_triangles()[0];
	const glm::vec3& p1 = trimesh->get_positions()[tri.indices[0]];
	const glm::vec3& p2 = trimesh->get_positions()[tri.indices[1]];
	const glm::vec3& p3 = trimesh->get_positions()[tri.indices[2]];
	const glm::vec3& v1 = p2 - p1;
	const glm::vec3& v2 = p3 - p1;

	normal = glm::normalize(glm::cross(v1, v2));
	position = p1;
}