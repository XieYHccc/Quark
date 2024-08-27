#include "Quark/qkpch.h"
#include "Quark/Scene/Components/MeshRendererCmpt.h"

namespace quark {
void MeshRendererCmpt::SetMesh(const Ref<Mesh>& mesh)
{
	m_Mesh = mesh;
}

void MeshRendererCmpt::SetMaterial(const Ref<Material>& mat, uint32_t index)
{
	if (index < m_Mesh->subMeshes.size() - 1)
		m_Mesh->subMeshes[index].material = mat;
	else
		CORE_LOGW("MeshRendererCmpt::SetMaterial: Index out of range");
}

Ref<Material> MeshRendererCmpt::GetMaterial(uint32_t index)
{
	if (index < m_Mesh->subMeshes.size() - 1)
		return m_Mesh->subMeshes[index].material;
	else
		CORE_LOGW("MeshRendererCmpt::GetMaterial: Index out of range");

	return nullptr;
}

};