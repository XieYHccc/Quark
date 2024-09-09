#pragma once
#include "Quark/Ecs/Component.h"
#include "Quark/Asset/Mesh.h"
#include "Quark/Asset/Material.h"
#include "Quark/Renderer/ShaderManager.h"

namespace quark {

class MeshRendererCmpt : public Component {
public:
	QK_COMPONENT_TYPE_DECL(MeshRendererCmpt)

	MeshRendererCmpt() = default;

	void SetMesh(const Ref<Mesh>& mesh);
	void SetMaterial(uint32_t index, const Ref<Material>& mat);

	Ref<Material> GetMaterial(uint32_t index);
	const std::vector<Ref<Material>>& GetMaterials() const { return m_Materials; }

private:
	// Calls from SceneRenderer
	Ref<graphic::PipeLine> GetGraphicsPipeLine(uint32_t index);

private:
	void UpdateCachedVertexAttribs(uint32_t meshAttribsMask);
	void UpdateGraphicsPipeLine(uint32_t index);

private:
	Ref<Mesh> m_Mesh;
	
	// The count of materials should be equal to the count of submeshes in the mesh
	std::vector<Ref<Material>> m_Materials;

	// TODO: Change this when we have a render graph system
	// A mesh could be processed through multiple render pass and multiple pipelines
	std::vector<Ref<graphic::PipeLine>> m_GraphicsPipeLines;

	VariantSignatureKey m_CachedProgramVatriantKey = {};
	
	std::vector<graphic::VertexAttribInfo> m_CachedVertexAttribs;
	std::vector<graphic::VertexBindInfo> m_CachedVertexBindInfos;

	friend class SceneRenderer;
};
}