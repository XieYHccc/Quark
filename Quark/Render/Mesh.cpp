#include "Quark/qkpch.h"
#include "Quark/Render/Mesh.h"
#include "Quark/Render/RenderQueue.h"
#include "Quark/Render/RenderSystem.h"
namespace quark
{
static Queue Material2Queue(const PBRMaterial& mat)
{
	if (mat.draw_pipeline == DrawPipeline::AlphaBlend)
		return Queue::Transparent;
	else
		return Queue::Opaque;
}

void StaticMesh::GetRenderData(const RenderContext& context, const RenderInfoCmpt* transform, RenderQueue& queue) const
{
	Queue queue_type= Material2Queue(*material);

	util::Hasher h;
	h.u32(mesh_attribute_mask);
	h.u32(util::ecast(material->draw_pipeline));
	h.u64(material->shader_program->GetHash());
	util::Hash pipeline_hash = h.get();

	QK_CORE_ASSERT(material->hash != 0);
	h.u64(material->hash);
	util::Hash material_hash = h.get(); // hash for a material
	h.pointer(mesh_buffers->vbo_position.get());
	util::Hash draw_hash = h.get(); // hash for a drawcall

	QK_CORE_ASSERT(hash != 0);
	uint64_t instance_key = hash;
	uint64_t sort_key = RenderInfo::GetSortKey(context, queue_type, pipeline_hash, material_hash, draw_hash, static_aabb.Transform(transform->world_transform).GetCenter());

	auto* instance_data = queue.AllocateOne<StaticMeshPerInstanceData>();
	instance_data->vertex.model = transform->world_transform;

	auto* perdrawcall_data = queue.PushTask<StaticMeshPerDrawcallData>(queue_type, instance_key, sort_key, StaticMeshRender, instance_data);

	if (perdrawcall_data)
	{
		FillPerDrawcallData(*perdrawcall_data);
		// TODO :Remvoe hard code
		perdrawcall_data->shader_program = RenderSystem::Get().GetRenderResourceManager().GetShaderLibrary().program_staticMesh;
	}
}

void StaticMesh::FillPerDrawcallData(StaticMeshPerDrawcallData& data) const
{
	data.vbo_position = mesh_buffers->vbo_position.get();
	data.vbo_varying = mesh_buffers->vbo_varying.get();
	data.vbo_varying_enable_blending = mesh_buffers->vbo_varying_enable_blending.get();
	// data.vbo_joint_binding = mesh_buffers->vbo_joint_binding.get();
	data.ibo = mesh_buffers->ibo.get();
	data.ibo_offset = ibo_offset;
	data.vertex_offset = vertex_offset;
	data.vertex_count = vertex_count;
	data.fragment.base_color = material->base_color;
	data.fragment.metallic = material->metallic_factor;
	data.fragment.roughness = material->roughness_factor;
	data.mesh_attribute_mask = mesh_attribute_mask;
	data.textures[util::ecast(TextureKind::Albedo)] = material->textures[util::ecast(TextureKind::Albedo)].get();
	data.textures[util::ecast(TextureKind::Normal)] = material->textures[util::ecast(TextureKind::Normal)].get();
	data.textures[util::ecast(TextureKind::MetallicRoughness)] = material->textures[util::ecast(TextureKind::MetallicRoughness)].get();
	data.textures[util::ecast(TextureKind::Emissive)] = material->textures[util::ecast(TextureKind::Emissive)].get();

	data.draw_pipeline = material->draw_pipeline;

}

void SkinnedMesh::GetRenderData(const RenderContext& context, const RenderInfoCmpt* transform, RenderQueue& queue) const
{
	Queue queue_type = Material2Queue(*material);

	util::Hasher h;
	h.u32(mesh_attribute_mask);
	h.u32(util::ecast(material->draw_pipeline));
	h.u64(material->shader_program->GetHash());
	util::Hash pipeline_hash = h.get();

	QK_CORE_ASSERT(material->hash != 0);
	h.u64(material->hash);
	util::Hash material_hash = h.get(); // hash for a material
	h.pointer(mesh_buffers->vbo_position.get());
	util::Hash draw_hash = h.get(); // hash for a drawcall

	QK_CORE_ASSERT(hash != 0);
	uint64_t instance_key = hash;
	uint64_t sort_key = RenderInfo::GetSortKey(context, queue_type, pipeline_hash, material_hash, draw_hash, static_aabb.Transform(transform->world_transform).GetCenter());

	QK_CORE_ASSERT(transform->has_skin);
	auto* instance_data = queue.AllocateOne<SkinnedMeshPerInstanceData>();
	
	instance_data->num_bones = transform->num_bones;
	instance_data->world_transforms = queue.AllocateMany<glm::mat4>(transform->num_bones);


}

void BindMeshState(rhi::CommandList& cmd, const StaticMeshPerDrawcallData& data)
{
	using namespace rhi;
	auto& render_resource_manager = RenderSystem::Get().GetRenderResourceManager();
	Ref<rhi::PipeLine> pipeline = RenderSystem::Get().GetRenderResourceManager().RequestGraphicsPSO(
		*(data.shader_program), cmd.GetCurrentRenderPassInfo(), data.mesh_attribute_mask,
		true, data.draw_pipeline
	);

	cmd.BindPipeLine(*pipeline.get());

	cmd.BindImage(1, 0, *data.textures[util::ecast(TextureKind::Albedo)], ImageLayout::SHADER_READ_ONLY_OPTIMAL);
	cmd.BindSampler(1, 0, *render_resource_manager.sampler_linear);
	//cmd.BindImage(2, 2, *data.textures[util::ecast(TextureKind::MetallicRoughness)], ImageLayout::SHADER_READ_ONLY_OPTIMAL);
	//cmd.BindSampler(2, 2, *render_resource_manager.sampler_linear);

	cmd.PushConstant(&data.fragment, 0, sizeof(StaticMeshFragment));

	cmd.BindVertexBuffer(0, *data.vbo_position, 0);
	cmd.BindVertexBuffer(1, *data.vbo_varying_enable_blending, 0);
	if (data.vbo_varying)
		cmd.BindVertexBuffer(2, *data.vbo_varying, 0);
	//if (data.vbo_joint_binding)
	//	cmd.BindVertexBuffer(3, *data.vbo_joint_binding, 0);
	cmd.BindIndexBuffer(*data.ibo, 0, IndexBufferFormat::UINT32);
}

void StaticMeshRender(rhi::CommandList& cmd, const RenderQueueTask* task, unsigned instance_count)
{
	const StaticMeshPerDrawcallData* perdrawcall_data = static_cast<const StaticMeshPerDrawcallData*>(task->perdrawcall_data);

	BindMeshState(cmd, *perdrawcall_data);

	unsigned to_render = 0;
	for (unsigned i = 0; i < instance_count; i += to_render)
	{
		to_render = std::min<unsigned>(StaticMeshVertex::max_instances, instance_count - i);

		//rhi::BufferDesc desc;
		//desc.domain = rhi::BufferMemoryDomain::CPU;
		//desc.size = sizeof(glm::mat4) * to_render;
		//desc.usageBits = rhi::BufferUsageBits::BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		//Ref<rhi::Buffer> instance_buffer = RenderSystem::Get().GetDevice()->CreateBuffer(desc);
		//glm::mat4* instance_data = static_cast<glm::mat4*>(instance_buffer->GetMappedDataPtr());
		glm::mat4* ptr = static_cast<glm::mat4*>(cmd.AllocateConstantData(2, 0, sizeof(glm::mat4) * to_render));
		for (unsigned j = 0; j < to_render; j++)
			ptr[j] = static_cast<const StaticMeshPerInstanceData*>(task[i + j].instance_data)->vertex.model;
		// cmd.BindUniformBuffer(2, 0, *instance_buffer.get(), 0, desc.size);

		if (perdrawcall_data->ibo)
			cmd.DrawIndexed(perdrawcall_data->vertex_count, to_render, perdrawcall_data->ibo_offset, perdrawcall_data->vertex_offset, 0);
		else
			cmd.Draw(perdrawcall_data->vertex_count, to_render, perdrawcall_data->vertex_offset, 0);

	}

}

}