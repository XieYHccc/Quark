#include "Quark/qkpch.h"
#include "Quark/Render/Mesh.h"
#include "Quark/Render/RenderQueue.h"
#include "Quark/Render/RenderSystem.h"

namespace quark
{
static Queue Material2Queue(const RenderPBRMaterial& mat)
{
	if (mat.drawPipeline == DrawPipeline::AlphaBlend)
		return Queue::Transparent;
	else
		return Queue::Opaque;
}

void StaticMesh::GetRenderData(const RenderContext& context, const RenderInfoCmpt* transform, RenderQueue& queue) const
{
	Queue queue_type= Material2Queue(*material);

	util::Hasher h;
	h.u32(mesh_attribute_mask);
	h.u32(util::ecast(material->drawPipeline));
	h.u64(material->shaderProgram->GetHash());
	util::Hash pipeline_hash = h.get();

	h.u64(material->hash);
	h.pointer(vbo_position.get());
	util::Hash draw_hash = h.get(); // hash for a drawcall

	QK_CORE_ASSERT(hash != 0);
	uint64_t instance_key = hash;
	uint64_t sort_key = RenderInfo::GetSortKey(context, queue_type, pipeline_hash, draw_hash, static_aabb.Transform(transform->world_transform).GetCenter());

	auto* instance_data = queue.AllocateOne<StaticMeshPerInstanceData>();
	instance_data->vertex.model = transform->world_transform;

	auto* perdrawcall_data = queue.PushTask<StaticMeshPerDrawcallData>(queue_type, instance_key, sort_key, StaticMeshRender, instance_data);

	if (perdrawcall_data)
	{
		FillPerDrawcallData(*perdrawcall_data);
		// TODO :Remvoe hard code
		perdrawcall_data->shader_program = RenderSystem::Get().GetRenderResourceManager().GetShaderLibrary().program_test;
	}
}

//uint64_t StaticMesh::Bake()
//{
//	util::Hasher h;
//	h.pointer(vbo_position.get());
//	if (vbo_varying)
//		h.pointer(vbo_varying.get());
//	if (vbo_varying_enable_blending)
//		h.pointer(vbo_varying_enable_blending.get());
//	if (vbo_joint_binding)
//		h.pointer(vbo_joint_binding.get());
//	if (ibo)
//	{
//		h.pointer(ibo.get());
//		h.u32(ibo_offset);
//	}
//
//	h.u32(vertex_offset);
//	h.u32(vertex_count);
//	h.u32(mesh_attribute_mask);
//	h.u64(material->hash);
//
//	return h.get();
//
//}


void StaticMesh::FillPerDrawcallData(StaticMeshPerDrawcallData& data) const
{
	data.vbo_position = vbo_position.get();
	data.vbo_varying = vbo_varying.get();
	data.vbo_varying_enable_blending = vbo_varying_enable_blending.get();
	data.vbo_joint_binding = vbo_joint_binding.get();
	data.ibo = ibo.get();
	data.ibo_offset = ibo_offset;
	data.vertex_offset = vertex_offset;
	data.vertex_count = vertex_count;
	data.fragment.base_color = material->colorFactors;
	data.fragment.metallic = material->metallicFactor;
	data.fragment.roughness = material->roughnessFactor;
	data.mesh_attribute_mask = mesh_attribute_mask;
	data.textures[util::ecast(TextureKind::Albedo)] = material->base_color_texture_image.get();
	data.textures[util::ecast(TextureKind::Normal)] = material->normal_texture_image.get();
	data.textures[util::ecast(TextureKind::MetallicRoughness)] = material->metallic_roughness_texture_image.get();
	data.textures[util::ecast(TextureKind::Emissive)] = material->emissive_texture_image.get();

	data.alpha_mode = material->alphaMode;

}

void BindMeshState(rhi::CommandList& cmd, const StaticMeshPerDrawcallData& data)
{
	using namespace rhi;
	auto& render_resource_manager = RenderSystem::Get().GetRenderResourceManager();
	Ref<rhi::PipeLine> pipeline = RenderSystem::Get().GetRenderResourceManager().RequestGraphicsPSO(
		*(data.shader_program), cmd.GetCurrentRenderPassInfo(), data.mesh_attribute_mask,
		true, data.alpha_mode
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
	if (data.vbo_joint_binding)
		cmd.BindVertexBuffer(3, *data.vbo_joint_binding, 0);
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

		rhi::BufferDesc desc;
		desc.domain = rhi::BufferMemoryDomain::CPU;
		desc.size = sizeof(glm::mat4) * to_render;
		desc.usageBits = rhi::BufferUsageBits::BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		Ref<rhi::Buffer> instance_buffer = RenderSystem::Get().GetDevice()->CreateBuffer(desc);
		glm::mat4* instance_data = static_cast<glm::mat4*>(instance_buffer->GetMappedDataPtr());
		for (unsigned j = 0; j < to_render; j++)
			instance_data[j] = static_cast<const StaticMeshPerInstanceData*>(task[i + j].instance_data)->vertex.model;
		cmd.BindUniformBuffer(2, 0, *instance_buffer.get(), 0, desc.size);

		if (perdrawcall_data->ibo)
			cmd.DrawIndexed(perdrawcall_data->vertex_count, to_render, perdrawcall_data->ibo_offset, perdrawcall_data->vertex_offset, 0);
		else
			cmd.Draw(perdrawcall_data->vertex_count, to_render, perdrawcall_data->vertex_offset, 0);

	}

}

}