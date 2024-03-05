#include "./mesh_collider.h"

#include "../basic/object.h"
#include "../render/components/mesh_filter.h"


void MeshCollider::awake() {
	auto mesh_fileter = get_object()->get_component<MeshFilter>();

	trimesh = mesh_fileter->trimesh();

}