#include "Quark/qkpch.h"
#include "Quark/Render/RenderSystem.h"
#include "Quark/Render/RenderParameters.h"
#include "Quark/RHI/Vulkan/Device_Vulkan.h"
#include "Quark/Asset/AssetManager.h"
#include "Quark/Asset/MeshAsset.h"
#include "Quark/Scene/Scene.h"
#include "Quark/Scene/Components/CommonCmpts.h"
#include "Quark/Scene/Components/TransformCmpt.h"
#include "Quark/Scene/Components/MeshRendererCmpt.h"
#include "Quark/Core/Util/Hash.h"

enum GlobalDescriptorSetBindings
{
    BINDING_GLOBAL_CAMERA_PARAMETERS = 0,
    BINDING_GLOBAL_RENDER_PARAMETERS = 1,
};

namespace quark {

using namespace rhi;

RenderSystem::RenderSystem(const RenderSystemConfig& config)
{
    rhi::DeviceConfig rhi_config;
    rhi_config.framesInFlight = 2;
#ifdef USE_VULKAN_DRIVER
    m_device = CreateRef<rhi::Device_Vulkan>(rhi_config);
#endif

    m_renderResourceManager = CreateScope<RenderResourceManager>(m_device);

    QK_CORE_LOGI_TAG("Rernderer", "RenderSystem Initialized");
}

RenderSystem::~RenderSystem()
{
    m_renderResourceManager.reset();
    m_device.reset();
}

void RenderSystem::BindCameraParameters(rhi::CommandList& cmd, const RenderContext& cxt)
{

    //rhi::BufferDesc desc;
    //desc.domain = rhi::BufferMemoryDomain::CPU;
    //desc.size = sizeof(CameraParameters);
    //desc.usageBits = rhi::BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    //Ref<rhi::Buffer> camera_ubo = m_device->CreateBuffer(desc);
    //CameraParameters* camera_ubo_mapped_data = (CameraParameters*)camera_ubo->GetMappedDataPtr();
    //*camera_ubo_mapped_data = cxt.GetCameraParameters();
    //cmd.BindUniformBuffer(0, 0, *camera_ubo, 0, desc.size);

    CameraParameters* mapped = (CameraParameters*)cmd.AllocateConstantData(0, BINDING_GLOBAL_CAMERA_PARAMETERS, sizeof(CameraParameters));
    *mapped = cxt.GetCameraParameters();
}

void RenderSystem::BindLightingParameters(rhi::CommandList& cmd, const RenderContext& cxt)
{
    auto* light_params = cxt.GetLightingParameters();
    if (!light_params)
        return;
    CombinedRenderParameters* mapped = (CombinedRenderParameters*)cmd.AllocateConstantData(0, BINDING_GLOBAL_RENDER_PARAMETERS, sizeof(CombinedRenderParameters));
    mapped->directional = light_params->directional;

}

void RenderSystem::Flush(rhi::CommandList& cmd, const RenderQueue& queue, const RenderContext& ctx)
{
    BindCameraParameters(cmd, ctx);
    BindLightingParameters(cmd, ctx);

    queue.Dispatch(Queue::Opaque, cmd);
}

// void RenderSystem::DrawSkybox(uint64_t env_map_id, rhi::CommandList* cmd)
// {
    //Ref<MeshAsset> cubeMesh = AssetManager::Get().mesh_cube;
    //// TODO: Remove this after change asset manager
    //if (!m_renderResourceManager->IsMeshAssetRegisterd(cubeMesh->GetAssetID()))
    //    m_renderResourceManager->CreateMeshRenderResouce(cubeMesh->GetAssetID());
    //
    //if (!m_renderResourceManager->IsImageAssetRegisterd(env_map_id))
    //    m_renderResourceManager->CreateImageRenderResource(env_map_id);

    //auto& cubeRenderMesh = m_renderResourceManager->GetRenderMesh(cubeMesh->GetAssetID());
    //Ref<rhi::Image> envMap = m_renderResourceManager->GetImage(env_map_id);
    //Ref<rhi::PipeLine> pipeline_skybox = m_renderResourceManager->RequestGraphicsPSO(
    //    *m_renderResourceManager->GetShaderLibrary().staticProgram_skybox,
    //    cmd->GetCurrentRenderPassInfo(),
    //    m_renderResourceManager->mesh_attrib_mask_skybox,
    //    false, AlphaMode::MODE_OPAQUE);

    //cmd->BindUniformBuffer(0, 0, *m_renderResourceManager->ubo_scene, 0, sizeof(UniformBufferObject_Scene));

    //cmd->BindImage(1, 0, *envMap, ImageLayout::SHADER_READ_ONLY_OPTIMAL);
    //cmd->BindSampler(1, 0, *m_renderResourceManager->sampler_cube);
    //cmd->BindVertexBuffer(0, *cubeRenderMesh.vertex_position_buffer, 0);
    //cmd->BindIndexBuffer(*cubeRenderMesh.index_buffer, 0, IndexBufferFormat::UINT32);

    //cmd->BindPipeLine(*pipeline_skybox);
    //cmd->DrawIndexed((uint32_t)cubeMesh->indices.size(), 1, 0, 0, 0);
// }

void RenderSystem::DrawGrid(rhi::CommandList* cmd)
{
    //Ref<rhi::PipeLine> infiniteGrid_pipeline = m_renderResourceManager->RequestGraphicsPSO(
    //    *m_renderResourceManager->GetShaderLibrary().staticProgram_infiniteGrid,
    //    cmd->GetCurrentRenderPassInfo(), 0, true, AlphaMode::MODE_OPAQUE);
    //
    //cmd->BindUniformBuffer(0, 0, *m_renderResourceManager->ubo_scene, 0, sizeof(UniformBufferObject_Scene));

    //cmd->BindPipeLine(*infiniteGrid_pipeline);
    //cmd->Draw(6, 1, 0, 0);
}

// void RenderSystem::DrawScene(const RenderScene& scene, const Visibility& vis, rhi::CommandList* cmd)
// {   
  //  uint16_t lastMaterialID = 0;
  //  uint16_t lastMeshID = 0;
  //  RenderPBRMaterial* lastMaterial = nullptr;
  //  RenderMesh* lastMesh = nullptr;

  //  std::vector<uint32_t> visible_object_indexes = vis.main_camera_visible_object_indexes;
  //  std::sort(visible_object_indexes.begin(), visible_object_indexes.end(), [&](const uint32_t& iA, const uint32_t& iB)
  //  {
  //      const RenderObject& A = scene.render_objects[iA];
  //      const RenderObject& B = scene.render_objects[iB];
  //      if (A.render_material_id == B.render_material_id)
  //          return A.render_mesh_id < B.render_mesh_id;
  //      else
  //          return A.render_material_id < B.render_material_id;
  //  });

  //  cmd->BindUniformBuffer(0, 0, *m_renderResourceManager->ubo_scene, 0, sizeof(UniformBufferObject_Scene));
  //  auto draw = [&](const RenderObject& obj)
  //  {
  //      // rebind material
  //      if (obj.render_material_id != lastMaterialID)
  //      {
  //          lastMaterialID = obj.render_material_id;
  //          lastMaterial = &m_renderResourceManager->GetRenderMaterial(obj.render_material_id);
  //          // (deprecated)cmd_list->BindUniformBuffer(1, 0, *lastMaterial->uniformBuffer, lastMaterial->uniformBufferOffset, sizeof(Material::UniformBufferBlock));
  //          cmd->BindImage(1, 1, *lastMaterial->textures[util::ecast(TextureKind::Albedo)], ImageLayout::SHADER_READ_ONLY_OPTIMAL);
  //          cmd->BindSampler(1, 1, *m_renderResourceManager->sampler_linear);
  //          cmd->BindImage(1, 2, *lastMaterial->textures[util::ecast(TextureKind::MetallicRoughness)], ImageLayout::SHADER_READ_ONLY_OPTIMAL);
  //          cmd->BindSampler(1, 2, *m_renderResourceManager->sampler_linear);

  //          PushConstants_Material materialPushConstants;
  //          materialPushConstants.colorFactors = lastMaterial->colorFactors;
  //          materialPushConstants.metallicFactor = lastMaterial->metallicFactor;
  //          materialPushConstants.roughnessFactor = lastMaterial->roughnessFactor;
  //          cmd->PushConstant(&materialPushConstants, sizeof(glm::mat4), sizeof(PushConstants_Material));
  //      }
  //      // rebind mesh buffers
  //      if (obj.render_mesh_id != lastMeshID)
		//{
		//	lastMeshID = obj.render_mesh_id;
		//	lastMesh = &m_renderResourceManager->GetRenderMesh(obj.render_mesh_id);
		//	cmd->BindVertexBuffer(0, *lastMesh->vertex_position_buffer, 0);
		//	cmd->BindVertexBuffer(1, *lastMesh->vertex_varying_enable_blending_buffer, 0);
  //          if (lastMesh->vertex_varying_buffer)
  //              cmd->BindVertexBuffer(2, *lastMesh->vertex_varying_buffer, 0);
  //          if (lastMesh->vertex_joint_binding_buffer)
  //              cmd->BindVertexBuffer(3, *lastMesh->vertex_joint_binding_buffer, 0);
		//	cmd->BindIndexBuffer(*lastMesh->index_buffer, 0, IndexBufferFormat::UINT32);
		//}

  //      // bind joint matrics
  //      if (obj.enable_vertex_blending)
  //      {
  //          cmd->BindStorageBuffer(2, 0, *obj.perdrawcall_ssbo_mesh_joint_matrices, 0, sizeof(PerDrawCall_StorageBufferObject_EnableMeshVertexBlending));
  //      }

  //      // push model constant
  //      PushConstants_Model push_constant;
  //      push_constant.worldMatrix = obj.model_matrix;

  //      cmd->PushConstant(&push_constant, 0, 64);  // only push model matrix

  //      // rebind Pipeline
  //      Ref<rhi::PipeLine> pipeline = m_renderResourceManager->RequestGraphicsPSO(
  //          *lastMaterial->shaderProgram, cmd->GetCurrentRenderPassInfo(),
  //          lastMesh->mesh_attribute_mask, true, lastMaterial->alphaMode);
  //      cmd->BindPipeLine(*pipeline);

  //      cmd->DrawIndexed(obj.index_count, 1, obj.start_index, 0, 0);
  //  };

  //  for (auto obj : vis.main_camera_visible_object_indexes)
  //      draw(scene.render_objects[obj]);
// }

//void RenderSystem::DrawEntityID(const RenderScene& scene, const Visibility& vis, rhi::CommandList* cmd)
//{
//    QK_CORE_ASSERT(cmd->GetCurrentRenderPassInfo().colorAttachmentFormats[0] == rhi::DataFormat::R32G32_UINT);
//    
//    cmd->BindUniformBuffer(0, 0, *m_renderResourceManager->ubo_scene, 0, sizeof(UniformBufferObject_Scene));
//    cmd->BindPipeLine(*m_renderResourceManager->pipeline_entityID);
//
//    for (const uint32_t idx : vis.main_camera_visible_object_indexes)
//    {
//        const RenderObject& obj = scene.render_objects[idx];
//        RenderMesh& render_mesh = m_renderResourceManager->GetRenderMesh(obj.render_mesh_id);
//        auto find = scene.render_object_to_entity.find(obj.id);
//        QK_CORE_ASSERT(find != scene.render_object_to_entity.end());
//        cmd->BindVertexBuffer(0, *render_mesh.vertex_position_buffer, 0);
//        cmd->BindIndexBuffer(*render_mesh.index_buffer, 0, IndexBufferFormat::UINT32);
//        cmd->PushConstant(&obj.model_matrix, 0, 64);
//        cmd->PushConstant(&find->second, 64, 8);
//
//        cmd->DrawIndexed(obj.index_count, 1, obj.start_index, 0, 0);
//    }
//
//}
}