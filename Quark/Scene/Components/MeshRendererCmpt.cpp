#include "Quark/qkpch.h"
#include "Quark/Renderer/Renderer.h"
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
	if (index < m_Materials.size())
	{
		m_Materials[index] = mat;
		m_DirtyMaterialMask |= 1 << index;
	}
	else
		QK_CORE_LOGW_TAG("Scene", "MeshRendererCmpt::SetMaterial: Index out of range");
}

Ref<Material> MeshRendererCmpt::GetMaterial(uint32_t index)
{
	QK_CORE_ASSERT(index < m_Materials.size())
	QK_CORE_ASSERT(m_Materials[index] != nullptr)
	
	return m_Materials[index];
}

Ref<graphic::PipeLine> MeshRendererCmpt::GetGraphicsPipeLine(uint32_t index)
{
	if (index >= m_GraphicsPipeLines.size())
	{
		QK_CORE_LOGW_TAG("Scene", "MeshRendererCmpt::GetPipeLine: Index out of range");
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

		if ((m_DirtyMaterialMask & (1u < index)) != 0)
		{
			m_DirtyMaterialMask &= ~(1u << index);
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
	m_CachedVertexInputLayout = {};

	// TODO: Add support for dynamic mesh
	// Position
	if (meshAttribsMask & MESH_ATTRIBUTE_POSITION_BIT)
	{
		graphic::VertexInputLayout::VertexAttribInfo& attrib = m_CachedVertexInputLayout.vertexAttribInfos.emplace_back();
		attrib.location = 0;
		attrib.binding = 0;
		attrib.format = graphic::VertexInputLayout::VertexAttribInfo::ATTRIB_FORMAT_VEC3;
		attrib.offset = 0;
	}

	uint32_t offset = 0;

	// UV
	if (meshAttribsMask & MESH_ATTRIBUTE_UV_BIT)
	{
		graphic::VertexInputLayout::VertexAttribInfo& attrib = m_CachedVertexInputLayout.vertexAttribInfos.emplace_back();
		attrib.location = 1;
		attrib.binding = 1;
		attrib.format = graphic::VertexInputLayout::VertexAttribInfo::ATTRIB_FORMAT_VEC2;
		attrib.offset = offset;
		offset += sizeof(decltype(m_Mesh->vertex_uvs)::value_type);
	}

	// Normal
	if (meshAttribsMask & MESH_ATTRIBUTE_NORMAL_BIT)
	{
		graphic::VertexInputLayout::VertexAttribInfo& attrib = m_CachedVertexInputLayout.vertexAttribInfos.emplace_back();
		attrib.location = 2;
		attrib.binding = 1;
		attrib.format = graphic::VertexInputLayout::VertexAttribInfo::ATTRIB_FORMAT_VEC3;
		attrib.offset = offset;
		offset += sizeof(decltype(m_Mesh->vertex_normals)::value_type);
	}

	// Vertex Color
	if (meshAttribsMask & MESH_ATTRIBUTE_VERTEX_COLOR_BIT)
	{
		graphic::VertexInputLayout::VertexAttribInfo& attrib = m_CachedVertexInputLayout.vertexAttribInfos.emplace_back();
		attrib.location = 3;
		attrib.binding = 1;
		attrib.format = graphic::VertexInputLayout::VertexAttribInfo::ATTRIB_FORMAT_VEC4;
		attrib.offset = offset;
		offset += sizeof(decltype(m_Mesh->vertex_colors)::value_type);
	}

	graphic::VertexInputLayout::VertexBindInfo bindInfo = {};
	bindInfo.binding = 0;
	bindInfo.stride = sizeof(decltype(m_Mesh->vertex_positions)::value_type);
	bindInfo.inputRate = graphic::VertexInputLayout::VertexBindInfo::INPUT_RATE_VERTEX;
	m_CachedVertexInputLayout.vertexBindInfos.push_back(bindInfo);

	bindInfo.binding = 1;
	bindInfo.stride = offset;
	bindInfo.inputRate = graphic::VertexInputLayout::VertexBindInfo::INPUT_RATE_VERTEX;
	m_CachedVertexInputLayout.vertexBindInfos.push_back(bindInfo);
}

void MeshRendererCmpt::UpdateGraphicsPipeLine(uint32_t index)
{
	Ref<Material> mat = GetMaterial(index);

	ShaderProgramVariant* programVariant = mat->shaderProgram->GetOrCreateVariant(m_CachedProgramVatriantKey);

	// TODO: Remove hardcoded states after restruct Material class
	graphic::PipelineDepthStencilState dss = mat->alphaMode == AlphaMode::OPAQUE ?
		Renderer::Get().depthStencilState_depthWrite: Renderer::Get().depthStencilState_depthTestOnly;

	graphic::PipelineColorBlendState cbs = mat->alphaMode == AlphaMode::OPAQUE ?
		graphic::PipelineColorBlendState::create_disabled(1) : graphic::PipelineColorBlendState::create_blend(1);

	m_GraphicsPipeLines[index] = programVariant->GetOrCreatePipeLine(dss, cbs,
		Renderer::Get().rasterizationState_fill,
		Renderer::Get().renderPassInfo2_simpleColorDepthPass, //TODO: Remove hardcoded render pass when we have render graph system
		m_CachedVertexInputLayout);
}

}
