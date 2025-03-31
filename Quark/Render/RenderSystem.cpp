#include "Quark/qkpch.h"
#include "Quark/Render/RenderSystem.h"
#include "Quark/Render/RenderParameters.h"
#include "Quark/RHI/Vulkan/Device_Vulkan.h"
#include "Quark/Asset/AssetManager.h"
#include "Quark/Asset/MeshAsset.h"
#include "Quark/Scene/Scene.h"
#include "Quark/Scene/Components/CommonCmpts.h"
#include "Quark/Scene/Components/MeshCmpt.h"
#include "Quark/Scene/Components/TransformCmpt.h"
#include "Quark/Scene/Components/MeshRendererCmpt.h"
#include "Quark/Core/Util/Hash.h"

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
    m_renderScene = CreateRef<RenderScene>();

    QK_CORE_LOGI_TAG("Rernderer", "RenderSystem Initialized");
}

RenderSystem::~RenderSystem()
{
    m_renderScene.reset();
    m_renderResourceManager.reset();
    m_device.reset();
}

void RenderSystem::ProcessSwapData()
{
    RenderSwapData& renderSwapData = m_swapContext.GetRenderSwapData();

    // update render entites
    if (!renderSwapData.dirty_static_mesh_render_proxies.empty()) 
    {
        for (const auto& renderProxy : renderSwapData.dirty_static_mesh_render_proxies)
        {
            for (size_t section_index = 0; section_index < renderProxy.mesh_sections.size(); section_index++)
            {   
                const MeshSectionDesc& section_desc = renderProxy.mesh_sections[section_index];
                RenderObject new_render_obj;

                // calculate entity's hash id
                util::Hasher hasher;
                hasher.u64(renderProxy.entity_id);
                hasher.u64(section_index);
                uint64_t entity_id = hasher.get();
                new_render_obj.id = entity_id;

                new_render_obj.model_matrix = renderProxy.transform;
                new_render_obj.aabb = section_desc.aabb;
                new_render_obj.render_mesh_id = renderProxy.mesh_asset_id;
                new_render_obj.render_material_id = section_desc.material_asset_id;
                new_render_obj.index_count = section_desc.index_count;
                new_render_obj.start_index = section_desc.index_offset;

                // create mesh render resources
                if (!m_renderResourceManager->IsMeshAssetRegisterd(renderProxy.mesh_asset_id)) 
                    m_renderResourceManager->CreateMeshRenderResouce(renderProxy.mesh_asset_id);

                if (section_desc.material_asset_id == 0)
                {
                    // use default material
                    new_render_obj.render_material_id = m_renderResourceManager->default_material_id;
                }
                else 
                {
                    // create material resource
                    if (!m_renderResourceManager->IsMaterialAssetRegisterd(section_desc.material_asset_id))
                        m_renderResourceManager->CreateMaterialRenderResource(section_desc.material_asset_id);
                }
                // TODO: REMOVE
                if (renderProxy.joint_matrices.size() > 0)
                {
                    new_render_obj.enable_vertex_blending = true;
                    rhi::BufferDesc joint_matrics_ssbo_desc;
                    joint_matrics_ssbo_desc.domain = rhi::BufferMemoryDomain::CPU;
                    joint_matrics_ssbo_desc.size = sizeof(PerDrawCall_StorageBufferObject_EnableMeshVertexBlending);
                    joint_matrics_ssbo_desc.usageBits = rhi::BUFFER_USAGE_STORAGE_BUFFER_BIT;
                    new_render_obj.perdrawcall_ssbo_mesh_joint_matrices = m_device->CreateBuffer(joint_matrics_ssbo_desc);
                    PerDrawCall_StorageBufferObject_EnableMeshVertexBlending& joint_matrices_ssbo_mapped_data =
                        *(PerDrawCall_StorageBufferObject_EnableMeshVertexBlending*)new_render_obj.perdrawcall_ssbo_mesh_joint_matrices->GetMappedDataPtr();
                    for (size_t i = 0; i < renderProxy.joint_matrices.size(); i++)
                    {
                        joint_matrices_ssbo_mapped_data.joint_matrices[i] = renderProxy.joint_matrices[i];
                    }
                }

                // add to render scene
                auto find = m_renderScene->render_object_to_offset.find(new_render_obj.id);
                if (find != m_renderScene->render_object_to_offset.end())
                {
                    m_renderScene->render_objects[find->second] = new_render_obj;
                }
                else
                {
                    m_renderScene->render_objects.push_back(new_render_obj);
                    m_renderScene->render_object_to_offset[new_render_obj.id] = m_renderScene->render_objects.size() - 1;
                    m_renderScene->render_object_to_entity[new_render_obj.id] = renderProxy.entity_id;
                }

            }
        }
        
        renderSwapData.dirty_static_mesh_render_proxies.clear();
    }

    // delete render entities
    if (!renderSwapData.to_delete_entities.empty())
    {
        for (const uint64_t entity_id : renderSwapData.to_delete_entities)
            m_renderScene->DeleteRenderObjectsByEntityID(entity_id);

        renderSwapData.to_delete_entities.clear();
    }

    // update camera
    if (renderSwapData.camera_swap_data.has_value())
    {
        m_renderScene->ubo_data_scene.cameraUboData.view = renderSwapData.camera_swap_data->view;
        m_renderScene->ubo_data_scene.cameraUboData.proj = renderSwapData.camera_swap_data->proj;
        m_renderScene->ubo_data_scene.cameraUboData.viewproj = renderSwapData.camera_swap_data->proj * renderSwapData.camera_swap_data->view;
        // update visibility TODO: Remove this
        m_renderScene->UpdateVisibility(m_renderScene->main_camera_visibility, m_renderScene->ubo_data_scene.cameraUboData);
        renderSwapData.camera_swap_data.reset();
    }

    m_renderResourceManager->UpdatePerFrameBuffer(m_renderScene);

}

void RenderSystem::BindCameraParameters(rhi::CommandList& cmd, const RenderContext& cxt)
{
    rhi::BufferDesc desc;
    desc.domain = rhi::BufferMemoryDomain::CPU;
    desc.size = sizeof(CameraParameters);
    desc.usageBits = rhi::BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    Ref<rhi::Buffer> camera_ubo = m_device->CreateBuffer(desc);
    CameraParameters* camera_ubo_mapped_data = (CameraParameters*)camera_ubo->GetMappedDataPtr();
    *camera_ubo_mapped_data = cxt.GetCameraParameters();

    cmd.BindUniformBuffer(0, 0, *camera_ubo, 0, desc.size);
}

void RenderSystem::Flush(rhi::CommandList& cmd, const RenderQueue& queue, const RenderContext& ctx)
{
    BindCameraParameters(cmd, ctx);

    queue.Dispatch(Queue::Opaque, cmd);
}

void RenderSystem::DrawSkybox(uint64_t env_map_id, rhi::CommandList* cmd)
{
    Ref<MeshAsset> cubeMesh = AssetManager::Get().mesh_cube;
    // TODO: Remove this after change asset manager
    if (!m_renderResourceManager->IsMeshAssetRegisterd(cubeMesh->GetAssetID()))
        m_renderResourceManager->CreateMeshRenderResouce(cubeMesh->GetAssetID());
    
    if (!m_renderResourceManager->IsImageAssetRegisterd(env_map_id))
        m_renderResourceManager->CreateImageRenderResource(env_map_id);

    auto& cubeRenderMesh = m_renderResourceManager->GetRenderMesh(cubeMesh->GetAssetID());
    Ref<rhi::Image> envMap = m_renderResourceManager->GetImage(env_map_id);
    Ref<rhi::PipeLine> pipeline_skybox = m_renderResourceManager->RequestGraphicsPSO(
        *m_renderResourceManager->GetShaderLibrary().staticProgram_skybox,
        cmd->GetCurrentRenderPassInfo(),
        m_renderResourceManager->mesh_attrib_mask_skybox,
        false, AlphaMode::MODE_OPAQUE);

    cmd->BindUniformBuffer(0, 0, *m_renderResourceManager->ubo_scene, 0, sizeof(UniformBufferObject_Scene));

    cmd->BindImage(1, 0, *envMap, ImageLayout::SHADER_READ_ONLY_OPTIMAL);
    cmd->BindSampler(1, 0, *m_renderResourceManager->sampler_cube);
    cmd->BindVertexBuffer(0, *cubeRenderMesh.vertex_position_buffer, 0);
    cmd->BindIndexBuffer(*cubeRenderMesh.index_buffer, 0, IndexBufferFormat::UINT32);

    cmd->BindPipeLine(*pipeline_skybox);
    cmd->DrawIndexed((uint32_t)cubeMesh->indices.size(), 1, 0, 0, 0);
}

void RenderSystem::DrawGrid(rhi::CommandList* cmd)
{
    Ref<rhi::PipeLine> infiniteGrid_pipeline = m_renderResourceManager->RequestGraphicsPSO(
        *m_renderResourceManager->GetShaderLibrary().staticProgram_infiniteGrid,
        cmd->GetCurrentRenderPassInfo(), 0, true, AlphaMode::MODE_OPAQUE);
    
    cmd->BindUniformBuffer(0, 0, *m_renderResourceManager->ubo_scene, 0, sizeof(UniformBufferObject_Scene));

    cmd->BindPipeLine(*infiniteGrid_pipeline);
    cmd->Draw(6, 1, 0, 0);
}

void RenderSystem::DrawScene(const RenderScene& scene, const Visibility& vis, rhi::CommandList* cmd)
{   
    uint16_t lastMaterialID = 0;
    uint16_t lastMeshID = 0;
    RenderPBRMaterial* lastMaterial = nullptr;
    RenderMesh* lastMesh = nullptr;

    std::vector<uint32_t> visible_object_indexes = vis.main_camera_visible_object_indexes;
    std::sort(visible_object_indexes.begin(), visible_object_indexes.end(), [&](const uint32_t& iA, const uint32_t& iB)
    {
        const RenderObject& A = scene.render_objects[iA];
        const RenderObject& B = scene.render_objects[iB];
        if (A.render_material_id == B.render_material_id)
            return A.render_mesh_id < B.render_mesh_id;
        else
            return A.render_material_id < B.render_material_id;
    });

    cmd->BindUniformBuffer(0, 0, *m_renderResourceManager->ubo_scene, 0, sizeof(UniformBufferObject_Scene));
    auto draw = [&](const RenderObject& obj)
    {
        // rebind material
        if (obj.render_material_id != lastMaterialID)
        {
            lastMaterialID = obj.render_material_id;
            lastMaterial = &m_renderResourceManager->GetRenderMaterial(obj.render_material_id);
            // (deprecated)cmd_list->BindUniformBuffer(1, 0, *lastMaterial->uniformBuffer, lastMaterial->uniformBufferOffset, sizeof(Material::UniformBufferBlock));
            cmd->BindImage(1, 1, *lastMaterial->base_color_texture_image, ImageLayout::SHADER_READ_ONLY_OPTIMAL);
            cmd->BindSampler(1, 1, *m_renderResourceManager->sampler_linear);
            cmd->BindImage(1, 2, *lastMaterial->metallic_roughness_texture_image, ImageLayout::SHADER_READ_ONLY_OPTIMAL);
            cmd->BindSampler(1, 2, *m_renderResourceManager->sampler_linear);

            PushConstants_Material materialPushConstants;
            materialPushConstants.colorFactors = lastMaterial->colorFactors;
            materialPushConstants.metallicFactor = lastMaterial->metallicFactor;
            materialPushConstants.roughnessFactor = lastMaterial->roughnessFactor;
            cmd->PushConstant(&materialPushConstants, sizeof(glm::mat4), sizeof(PushConstants_Material));
        }
        // rebind mesh buffers
        if (obj.render_mesh_id != lastMeshID)
		{
			lastMeshID = obj.render_mesh_id;
			lastMesh = &m_renderResourceManager->GetRenderMesh(obj.render_mesh_id);
			cmd->BindVertexBuffer(0, *lastMesh->vertex_position_buffer, 0);
			cmd->BindVertexBuffer(1, *lastMesh->vertex_varying_enable_blending_buffer, 0);
            if (lastMesh->vertex_varying_buffer)
                cmd->BindVertexBuffer(2, *lastMesh->vertex_varying_buffer, 0);
            if (lastMesh->vertex_joint_binding_buffer)
                cmd->BindVertexBuffer(3, *lastMesh->vertex_joint_binding_buffer, 0);
			cmd->BindIndexBuffer(*lastMesh->index_buffer, 0, IndexBufferFormat::UINT32);
		}

        // bind joint matrics
        if (obj.enable_vertex_blending)
        {
            cmd->BindStorageBuffer(2, 0, *obj.perdrawcall_ssbo_mesh_joint_matrices, 0, sizeof(PerDrawCall_StorageBufferObject_EnableMeshVertexBlending));
        }

        // push model constant
        PushConstants_Model push_constant;
        push_constant.worldMatrix = obj.model_matrix;

        cmd->PushConstant(&push_constant, 0, 64);  // only push model matrix

        // rebind Pipeline
        Ref<rhi::PipeLine> pipeline = m_renderResourceManager->RequestGraphicsPSO(
            *lastMaterial->shaderProgram, cmd->GetCurrentRenderPassInfo(),
            lastMesh->mesh_attribute_mask, true, lastMaterial->alphaMode);
        cmd->BindPipeLine(*pipeline);

        cmd->DrawIndexed(obj.index_count, 1, obj.start_index, 0, 0);
    };

    for (auto obj : vis.main_camera_visible_object_indexes)
        draw(scene.render_objects[obj]);
}

void RenderSystem::DrawEntityID(const RenderScene& scene, const Visibility& vis, rhi::CommandList* cmd)
{
    QK_CORE_ASSERT(cmd->GetCurrentRenderPassInfo().colorAttachmentFormats[0] == rhi::DataFormat::R32G32_UINT);
    
    cmd->BindUniformBuffer(0, 0, *m_renderResourceManager->ubo_scene, 0, sizeof(UniformBufferObject_Scene));
    cmd->BindPipeLine(*m_renderResourceManager->pipeline_entityID);

    for (const uint32_t idx : vis.main_camera_visible_object_indexes)
    {
        const RenderObject& obj = scene.render_objects[idx];
        RenderMesh& render_mesh = m_renderResourceManager->GetRenderMesh(obj.render_mesh_id);
        auto find = scene.render_object_to_entity.find(obj.id);
        QK_CORE_ASSERT(find != scene.render_object_to_entity.end());
        cmd->BindVertexBuffer(0, *render_mesh.vertex_position_buffer, 0);
        cmd->BindIndexBuffer(*render_mesh.index_buffer, 0, IndexBufferFormat::UINT32);
        cmd->PushConstant(&obj.model_matrix, 0, 64);
        cmd->PushConstant(&find->second, 64, 8);

        cmd->DrawIndexed(obj.index_count, 1, obj.start_index, 0, 0);
    }

}
}