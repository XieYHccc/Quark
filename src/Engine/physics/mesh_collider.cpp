#include "./mesh_collider.h"

#include "../Object/Object.h"
#include "../Object/Components/MeshFilterCmpt/MeshFilterCmpt.h"


void MeshCollider::awake() {
	auto mesh_fileter = get_object()->get_component<MeshFilterCmpt>();

	trimesh = mesh_fileter->trimesh();

}