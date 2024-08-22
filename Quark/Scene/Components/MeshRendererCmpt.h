#pragma once
#include "Quark/Ecs/Component.h"
#include "Quark/Asset/Mesh.h"
#include "Quark/Asset/Material.h"

namespace quark {

class MeshRendererCmpt : public Component {
public:
	QK_COMPONENT_TYPE_DECL(MeshRendererCmpt)

	MeshRendererCmpt() = default;

	void SetMesh(const Ref<Mesh>& mesh);
	void SetMaterial(const Ref<Material>& mat, uint32_t index);

	Ref<Material> GetMaterial(uint32_t index);

private:
	Ref<Mesh> m_Mesh;
};
}