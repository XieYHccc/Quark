#include "./mesh_collider.h"

#include <rttr/registration.h>

#include "../basic/object.h"
#include "../render/components/mesh_filter.h"

using namespace rttr;
RTTR_REGISTRATION
{
	registration::class_<MeshCollider>("MeshCollider")
			.constructor<>()(rttr::policy::ctor::as_raw_ptr);
}

void MeshCollider::awake() {
	auto mesh_fileter = dynamic_cast<MeshFilter*>(get_object()->get_component("MeshFilter"));

	trimesh = mesh_fileter->trimesh();

}