#pragma once
#include "Quark/Render/IRenderable.h"
#include "Quark/Render/Material.h"
#include "Quark/Asset/MeshAsset.h"
#include "Quark/RHI/Common.h"

namespace quark
{
class ShaderProgramVariant;

struct StaticMeshVertex
{
	glm::mat4 model;
	glm::mat4* prevModel = nullptr;
	//mat4 Normal;
	enum
	{
		max_instances = 256
	};
};

struct StaticMeshFragment
{
	glm::vec4 base_color;
	glm::vec4 emissive;
	float roughness;
	float metallic;
	float normal_scale;
};

struct StaticMeshPerDrawcallData
{
	const rhi::Buffer* vbo_position;
	const rhi::Buffer* vbo_varying_enable_blending;
	const rhi::Buffer* vbo_varying;
	const rhi::Buffer* vbo_joint_binding;
	const rhi::Buffer* ibo;
	const rhi::Image* textures[util::ecast(TextureKind::Count)];
	ShaderProgram* shader_program;	// TODO: use ShaderProgramVariant

	uint32_t ibo_offset = 0;
	uint32_t vertex_offset = 0;
	uint32_t vertex_count = 0;
	uint32_t mesh_attribute_mask = 0;
	StaticMeshFragment fragment;
	DrawPipeline draw_pipeline;

	//bool two_sided;
	//bool alpha_test;
};

struct StaticMeshPerInstanceData
{
	StaticMeshVertex vertex;
};

void StaticMeshRender(rhi::CommandList& cmd, const RenderQueueTask*, unsigned instance_count);

struct StaticMesh : public IRenderable
{
	Ref<rhi::Buffer> vbo_position;
	Ref<rhi::Buffer> vbo_varying_enable_blending; // normal, tangent..
	Ref<rhi::Buffer> vbo_varying; // uv, color...
	Ref<rhi::Buffer> vbo_joint_binding; // for skinned mesh
	Ref<rhi::Buffer> ibo;

	uint32_t ibo_offset = 0;
	uint32_t vertex_offset = 0;
	uint32_t vertex_count = 0;
	uint32_t mesh_attribute_mask = 0;
	uint32_t hash = 0;
	Ref<PBRMaterial> material;
	math::Aabb static_aabb;

	void GetRenderData(const RenderContext& context, const RenderInfoCmpt* transform,
		RenderQueue& queue) const override;

	bool HasStaticAabb() const override { return true; }

	const math::Aabb* GetStaticAabb() const override { return &static_aabb; }

	DrawPipeline GetMeshDrawPipeline() const { return material->draw_pipeline; }

private:
	void FillPerDrawcallData(StaticMeshPerDrawcallData& data) const;

	uint64_t m_cached_hash = 0;
};


}