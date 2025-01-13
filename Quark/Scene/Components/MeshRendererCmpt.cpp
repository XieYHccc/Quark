#include "Quark/qkpch.h"
#include "Quark/Render/RenderSystem.h"
#include "Quark/Scene/Components/MeshRendererCmpt.h"

namespace quark {

void MeshRendererCmpt::SetMesh(const Ref<MeshAsset>& mesh)
{
	m_mesh = mesh;
	m_material_ids.assign(mesh->subMeshes.size(), 0);

}

void MeshRendererCmpt::SetMaterial(uint32_t index, AssetID id)
{
	if (index < m_material_ids.size())
	{
		m_material_ids[index] = id;
		m_dirtyMaterialMask |= 1 << index;
	}
	else
		QK_CORE_LOGW_TAG("Scene", "MeshRendererCmpt::SetMaterial: Index out of range");
}

AssetID MeshRendererCmpt::GetMaterialID(uint32_t index)
{
	QK_CORE_ASSERT(index < m_material_ids.size())
	return m_material_ids[index];
}

//Ref<rhi::PipeLine> MeshRendererCmpt::GetGraphicsPipeLine(uint32_t index)
//{
//	QK_CORE_ASSERT(index < m_graphicsPipeLines.size())
//
//	if (m_graphicsPipeLines[index] == nullptr)
//	{
//		// create a new pipeline
//		m_cachedProgramVatriantKey.meshAttributeMask = m_mesh->GetMeshAttributeMask();
//		UpdateGraphicsPipeLine(index);
//	}
//	else
//	{
//		bool requireNewPipeline = false;
//
//		// check if mesh's attribute mask has changed
//		if (m_cachedProgramVatriantKey.meshAttributeMask != m_mesh->GetMeshAttributeMask())
//		{
//			m_cachedProgramVatriantKey.meshAttributeMask = m_mesh->GetMeshAttributeMask();
//			requireNewPipeline = true;
//		}
//
//		if ((m_dirtyMaterialMask & (1u < index)) != 0)
//		{
//			m_dirtyMaterialMask &= ~(1u << index);
//			requireNewPipeline = true;
//		}
//
//		if (requireNewPipeline)
//		{
//			UpdateGraphicsPipeLine(index);
//		}
//	}
//
//	return m_graphicsPipeLines[index];
//
//}

void MeshRendererCmpt::SetDirty(bool dirty)
{
	m_dirty = dirty;
}
}
