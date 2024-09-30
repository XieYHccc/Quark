#pragma once
#include "Quark/Ecs/Component.h"
#include "Quark/Asset/Mesh.h"

namespace quark {
struct MeshCmpt : public Component {
    // Use mesh if it exists, otherwise use sharedMesh
    Ref<Mesh> sharedMesh;
    Ref<Mesh> uniqueMesh;

    QK_COMPONENT_TYPE_DECL(MeshCmpt)
    using Component::Component;
};

// If you need to change the mesh's vertex data, use this
struct DynamicMeshCmpt : public Component {
	Ref<Mesh> mesh;

	QK_COMPONENT_TYPE_DECL(DynamicMeshCmpt)
	using Component::Component;
};

// Mesh's vertex data is static(won't be changed) after being generated
struct StaticMeshCmpt : public Component {
	Ref<Mesh> mesh;

	QK_COMPONENT_TYPE_DECL(StaticMeshCmpt)
	using Component::Component;
};


}