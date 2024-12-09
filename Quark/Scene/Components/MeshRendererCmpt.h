#pragma once
#include "Quark/Ecs/Component.h"
#include "Quark/Asset/Mesh.h"
#include "Quark/Asset/Material.h"
#include "Quark/RHI/PipeLine.h"
#include "Quark/Render/ShaderLibrary.h"

namespace quark {

class MeshRendererCmpt : public Component {
public:
	QK_COMPONENT_TYPE_DECL(MeshRendererCmpt)

	MeshRendererCmpt() = default;

	void SetMesh(const Ref<Mesh>& mesh);
	void SetMaterial(uint32_t index, const Ref<Material>& mat);

	Ref<Material> GetMaterial(uint32_t index);
	Ref<rhi::PipeLine> GetGraphicsPipeLine(uint32_t index);

private:
	void UpdateGraphicsPipeLine(uint32_t index);

private:
	Ref<Mesh> m_mesh;
	
	// The count of materials should be equal to the count of submeshes in the mesh
	std::vector<Ref<Material>> m_materials;
	uint32_t m_dirtyMaterialMask = 0;

	// TODO: Change this when we have a render graph system
	// A mesh could be processed through multiple render pass and multiple pipelines
	std::vector<Ref<rhi::PipeLine>> m_graphicsPipeLines;

	// Local rendering state
	ShaderVariantKey m_cachedProgramVatriantKey;


};
}