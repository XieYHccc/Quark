#pragma once
#include "Quark/Ecs/Component.h"
#include "Quark/Asset/MeshAsset.h"

namespace quark {
struct MeshCmpt : public Component {
    // Use mesh if it exists, otherwise use sharedMesh
    Ref<MeshAsset> sharedMesh;
    Ref<MeshAsset> uniqueMesh;

    QK_COMPONENT_TYPE_DECL(MeshCmpt)
    using Component::Component;
};

// If you need to change the mesh's vertex data, use this
struct DynamicMeshCmpt : public Component {
	Ref<MeshAsset> mesh;

	QK_COMPONENT_TYPE_DECL(DynamicMeshCmpt)
	using Component::Component;
};

// Mesh's vertex data is static(won't be changed) after being generated
struct StaticMeshCmpt : public Component {
	Ref<MeshAsset> mesh;

	QK_COMPONENT_TYPE_DECL(StaticMeshCmpt)
	using Component::Component;
};


}