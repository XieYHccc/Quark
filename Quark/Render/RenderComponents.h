#pragma once
#include "Quark/Ecs/Component.h"

namespace quark 
{
class MeshAsset;
class IRenderable;
 
struct RenderInfoCmpt : public Component
{
	QK_COMPONENT_TYPE_DECL(RenderInfoCmpt)
	glm::mat4 world_transform;

	bool has_skin = false;
	std::vector<glm::mat4> joint_matrices;
};

struct RenderableCmpt : public Component
{
	QK_COMPONENT_TYPE_DECL(RenderableCmpt)
	Ref<IRenderable> renderable;
};

struct StaticMeshCmpt : public Component
{
	QK_COMPONENT_TYPE_DECL(StaticMeshCmpt)
	Ref<MeshAsset> mesh_asset;
	std::vector<RenderableCmpt*> submesh_renderables;
};

struct OpaqueCmpt : public Component
{
	QK_COMPONENT_TYPE_DECL(OpaqueCmpt)
};

struct TransparentCmpt : public Component
{
	QK_COMPONENT_TYPE_DECL(TransparentCmpt)
};

}