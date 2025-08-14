#pragma once
#include "Quark/Ecs/Component.h"

#include <glm/glm.hpp>

namespace quark 
{
class MeshAsset;
class IRenderable;
class Skybox;

struct RenderInfoCmpt : public Component
{
	QK_COMPONENT_TYPE_DECL(RenderInfoCmpt)
	glm::mat4 world_transform;

	bool has_skin = false;
	uint32_t num_bones = 0;
	std::vector<glm::mat4> joint_matrices;

};

struct RenderableCmpt : public Component
{
	QK_COMPONENT_TYPE_DECL(RenderableCmpt)
	Ref<IRenderable> renderable;
};

struct MeshCmpt : public Component
{
	QK_COMPONENT_TYPE_DECL(MeshCmpt)
	Ref<MeshAsset> mesh_asset;
	std::vector<RenderableCmpt*> submeshes;
};

struct OpaqueCmpt : public Component
{
	QK_COMPONENT_TYPE_DECL(OpaqueCmpt)
};

struct TransparentCmpt : public Component
{
	QK_COMPONENT_TYPE_DECL(TransparentCmpt)
};

struct BackGroundCmpt : public Component
{
	QK_COMPONENT_TYPE_DECL(BackGroundCmpt)
};
}