#include "Quark/qkpch.h"
#include "Quark/Render/RenderSystem.h"
#include "Quark/Scene/Components/MeshRendererCmpt.h"

namespace quark {

void MeshRendererCmpt::SetMesh(const Ref<Mesh>& mesh)
{
	m_mesh = mesh;
	m_materials.resize(mesh->subMeshes.size());
	m_material_ids.resize(mesh->subMeshes.size());
	m_graphicsPipeLines.resize(mesh->subMeshes.size());
}

void MeshRendererCmpt::SetMaterial(uint32_t index, const Ref<Material>& mat)
{
	if (index < m_materials.size())
	{
		m_materials[index] = mat;
		m_dirtyMaterialMask |= 1 << index;
	}
	else
		QK_CORE_LOGW_TAG("Scene", "MeshRendererCmpt::SetMaterial: Index out of range");
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

Ref<Material> MeshRendererCmpt::GetMaterial(uint32_t index)
{
	QK_CORE_ASSERT(index < m_materials.size())
	QK_CORE_ASSERT(m_materials[index] != nullptr)
	
	return m_materials[index];
}

AssetID MeshRendererCmpt::GetMaterialID(uint32_t index)
{
	QK_CORE_ASSERT(index < m_material_ids.size())
	return m_material_ids[index];
}

Ref<rhi::PipeLine> MeshRendererCmpt::GetGraphicsPipeLine(uint32_t index)
{
	QK_CORE_ASSERT(index < m_graphicsPipeLines.size())

	if (m_graphicsPipeLines[index] == nullptr)
	{
		// create a new pipeline
		m_cachedProgramVatriantKey.meshAttributeMask = m_mesh->GetMeshAttributeMask();
		UpdateGraphicsPipeLine(index);
	}
	else
	{
		bool requireNewPipeline = false;

		// check if mesh's attribute mask has changed
		if (m_cachedProgramVatriantKey.meshAttributeMask != m_mesh->GetMeshAttributeMask())
		{
			m_cachedProgramVatriantKey.meshAttributeMask = m_mesh->GetMeshAttributeMask();
			requireNewPipeline = true;
		}

		if ((m_dirtyMaterialMask & (1u < index)) != 0)
		{
			m_dirtyMaterialMask &= ~(1u << index);
			requireNewPipeline = true;
		}

		if (requireNewPipeline)
		{
			UpdateGraphicsPipeLine(index);
		}
	}

	return m_graphicsPipeLines[index];

}

void MeshRendererCmpt::UpdateGraphicsPipeLine(uint32_t index)
{
	auto& renderer = RenderSystem::Get();

	Ref<Material> mat = GetMaterial(index);
	Ref<rhi::VertexInputLayout> vertexLayout = renderer.GetRenderResourceManager().GetOrCreateVertexInputLayout(m_cachedProgramVatriantKey.meshAttributeMask);

	m_graphicsPipeLines[index] = renderer.GetRenderResourceManager().GetOrCreateGraphicsPipeline(*(mat->shaderProgram), m_cachedProgramVatriantKey, renderer.GetRenderResourceManager().renderPassInfo_simpleMainPass, *vertexLayout, true, mat->alphaMode);
}

void MeshRendererCmpt::SetDirty(bool dirty)
{
	m_dirty = dirty;
}
}
