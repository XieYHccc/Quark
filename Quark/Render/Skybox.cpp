#include "Quark/qkpch.h"
#include "Quark/Render/Skybox.h"
#include "Quark/Render/RenderSystem.h"
#include "Quark/Asset/AssetManager.h"
namespace quark
{

void SkyboxRender(rhi::CommandList& cmd, const RenderQueueTask* task, unsigned instance_count)
{
	using namespace rhi;
	auto& resource_manager = RenderSystem::Get().GetRenderResourceManager();

	Ref<MeshAsset> cubeMesh = AssetManager::Get().mesh_cube;
	Ref<MeshBuffers> cubeRenderMesh = resource_manager.RequestMeshBuffers(cubeMesh);

	SkyboxPerDrawCallData* data = (SkyboxPerDrawCallData*)task->perdrawcall_data;

	Ref<rhi::PipeLine> pipeline_skybox = resource_manager.RequestGraphicsPSO(
		*resource_manager.GetShaderLibrary().program_skybox,
		cmd.GetCurrentRenderPassInfo(),
		resource_manager.mesh_attrib_mask_skybox,
		true, DrawPipeline::AlphaTest); // TODO: should be opaque here, change the Request PSO Interface in the future.

	cmd.BindImage(2, 0, *data->cubemap, ImageLayout::SHADER_READ_ONLY_OPTIMAL);
	cmd.BindSampler(2, 0, *resource_manager.sampler_cube);
	cmd.BindVertexBuffer(0, *cubeRenderMesh->vbo_position, 0);
	cmd.BindIndexBuffer(*cubeRenderMesh->ibo, 0, IndexBufferFormat::UINT32);

	cmd.BindPipeLine(*pipeline_skybox);
	cmd.DrawIndexed((uint32_t)cubeMesh->indices.size(), 1, 0, 0, 0);

}
void Skybox::GetRenderData(const RenderContext& context, const RenderInfoCmpt* transform, RenderQueue& queue) const
{
	util::Hasher hasher;
	
	if (m_cubemap)
		hasher.u64(m_cubemap->GetAssetID());
	else
		hasher.u32(0);

	auto instance_key = hasher.get();
	auto sorting_key = BuiltInSortKey::GetBackgroundSortKey(Queue::OpaqueEmissive, 0, 0);

	SkyboxPerDrawCallData* sky_data = queue.PushTask<SkyboxPerDrawCallData>(Queue::OpaqueEmissive, instance_key, sorting_key, SkyboxRender, nullptr);

	if (sky_data)
	{
		if (m_cubemap)
			sky_data->cubemap = RenderSystem::Get().GetRenderResourceManager().RequestImage(m_cubemap).get();
		else
			sky_data->cubemap = nullptr;

	}
}


}