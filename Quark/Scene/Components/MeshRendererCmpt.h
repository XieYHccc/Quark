#pragma once
#include "Quark/Ecs/Component.h"
#include "Quark/Asset/MeshAsset.h"
#include "Quark/RHI/PipeLine.h"
#include "Quark/Render/ShaderLibrary.h"

namespace quark {

//class MeshRendererCmpt : public Component {
//public:
//	QK_COMPONENT_TYPE_DECL(MeshRendererCmpt)
//
//	MeshRendererCmpt() = default;
//
//	void SetMesh(const Ref<MeshAsset>& mesh);
//	void SetMaterial(uint32_t index, AssetID id);
//	void SetDirty(bool dirty);
//	bool IsRenderStateDirty() const { return m_dirty; }
//
//	AssetID GetMaterialID(uint32_t index);
//	
//private:
//	Ref<MeshAsset> m_mesh;
//	
//	bool m_dirty = true;
//
//	// The count of materials should be equal to the count of submeshes in the mesh
//	std::vector<AssetID>  m_material_ids;
//
//	uint32_t m_dirtyMaterialMask = 0;
//
//
//};
}