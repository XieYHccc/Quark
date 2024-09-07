#include "Quark/qkpch.h"
#include "Quark/Renderer/GpuResourceManager.h"
#include "Quark/Scene/Components/MeshRendererCmpt.h"

namespace quark {

void MeshRendererCmpt::SetMesh(const Ref<Mesh>& mesh)
{
	m_Mesh = mesh;
	m_Materials.resize(mesh->subMeshes.size());
	m_GraphicsPipeLines.resize(mesh->subMeshes.size());
	
}

void MeshRendererCmpt::SetMaterial(uint32_t index, const Ref<Material>& mat)
{
	if(index < m_Materials.size() )
		m_Materials[index] = mat;
	else
		CORE_LOGW("MeshRendererCmpt::SetMaterial: Index out of range");
}

Ref<Material> MeshRendererCmpt::GetMaterial(uint32_t index)
{
	if (index < m_Materials.size())
		return m_Materials[index];
	else
		CORE_LOGW("MeshRendererCmpt::GetMaterial: Index out of range");

	return nullptr;
}

Ref<graphic::PipeLine> MeshRendererCmpt::GetGraphicsPipeLine(uint32_t index)
{
	if (index >= m_GraphicsPipeLines.size())
	{
		CORE_LOGW("MeshRendererCmpt::GetPipeLine: Index out of range");
		return nullptr;
	}

	if (m_GraphicsPipeLines[index] == nullptr)
	{
		// Create a new pipeline
		m_CachedProgramVatriantKey.meshAttributeMask = m_Mesh->GetMeshAttributeMask();

		UpdateCachedVertexAttribs(m_CachedProgramVatriantKey.meshAttributeMask);

		UpdateGraphicsPipeLine(index);

	}
	else
	{
		bool requireNewPipeline = false;

		// Check if mesh's attribute mask has changed
		if (m_CachedProgramVatriantKey.meshAttributeMask != m_Mesh->GetMeshAttributeMask())
		{
			m_CachedProgramVatriantKey.meshAttributeMask = m_Mesh->GetMeshAttributeMask();
			UpdateCachedVertexAttribs(m_CachedProgramVatriantKey.meshAttributeMask);

			requireNewPipeline = true;
		}

		if (requireNewPipeline)
		{
			UpdateGraphicsPipeLine(index);
		}

	}

	return m_GraphicsPipeLines[index];

}

void MeshRendererCmpt::UpdateCachedVertexAttribs(uint32_t meshAttribsMask)
{
	m_CachedVertexAttribs.clear();

	size_t offset = 0;

	// Position
	if (meshAttribsMask & MESH_ATTRIBUTE_POSITION_BIT)
	{
		graphic::VertexAttribInfo attrib = m_CachedVertexAttribs.emplace_back();
		attrib.location = 0;
		attrib.binding = 0;
		attrib.format = graphic::VertexAttribInfo::ATTRIB_FORMAT_VEC3;
		attrib.offset = offset;
		offset += sizeof(decltype(m_Mesh->positions)::value_type);
	}

	// UV
	if (meshAttribsMask & MESH_ATTRIBUTE_UV_BIT)
	{
		graphic::VertexAttribInfo attrib = m_CachedVertexAttribs.emplace_back();
		attrib.location = 1;
		attrib.binding = 0;
		attrib.format = graphic::VertexAttribInfo::ATTRIB_FORMAT_VEC2;
		attrib.offset = offset;
		offset += sizeof(decltype(m_Mesh->uvs)::value_type);
	}

	// Normal
	if (meshAttribsMask & MESH_ATTRIBUTE_NORMAL_BIT)
	{
		graphic::VertexAttribInfo attrib = m_CachedVertexAttribs.emplace_back();
		attrib.location = 2;
		attrib.binding = 0;
		attrib.format = graphic::VertexAttribInfo::ATTRIB_FORMAT_VEC3;
		attrib.offset = offset;
		offset += sizeof(decltype(m_Mesh->normals)::value_type);
	}

	// Vertex Color
	if (meshAttribsMask & MESH_ATTRIBUTE_VERTEX_COLOR_BIT)
	{
		graphic::VertexAttribInfo attrib = m_CachedVertexAttribs.emplace_back();
		attrib.location = 3;
		attrib.binding = 0;
		attrib.format = graphic::VertexAttribInfo::ATTRIB_FORMAT_VEC4;
		attrib.offset = offset;
		offset += sizeof(decltype(m_Mesh->colors)::value_type);
	}
}

void MeshRendererCmpt::UpdateGraphicsPipeLine(uint32_t index)
{
	ShaderProgramVariant* programVariant = m_Materials[index]->shaderProgram->GetOrCreateVariant(m_CachedProgramVatriantKey);

	// TODO: Remove hardcoded states after resruct Material class
	graphic::PipelineDepthStencilState dss = m_Materials[index]->alphaMode == AlphaMode::OPAQUE ?
		GpuResourceManager::Get().depthTestWriteState : GpuResourceManager::Get().depthTestState;

	graphic::PipelineColorBlendState cbs = m_Materials[index]->alphaMode == AlphaMode::OPAQUE ?
		graphic::PipelineColorBlendState::create_disabled(1) : graphic::PipelineColorBlendState::create_blend(1);

	m_GraphicsPipeLines[index] = programVariant->GetOrCreatePipeLine(dss, cbs,
		GpuResourceManager::Get().defaultFillRasterizationState,
		GpuResourceManager::Get().defaultOneColorWithDepthRenderPassInfo);
}

}
