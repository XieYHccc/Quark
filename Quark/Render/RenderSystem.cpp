#include "Quark/qkpch.h"
#include "Quark/Render/RenderSystem.h"
#include "Quark/Asset/AssetManager.h"
#include "Quark/Asset/MeshAsset.h"
#include "Quark/RHI/Device.h"
#include "Quark/Scene/Scene.h"
#include "Quark/Scene/Components/CommonCmpts.h"
#include "Quark/Scene/Components/MeshCmpt.h"
#include "Quark/Scene/Components/TransformCmpt.h"
#include "Quark/Scene/Components/MeshRendererCmpt.h"
#include "Quark/Core/Util/Hash.h"

namespace quark {

using namespace rhi;

RenderSystem::RenderSystem(Ref<rhi::Device> device)
    : m_device(device)
{
    m_renderResourceManager = CreateScope<RenderResourceManager>(m_device);

    m_renderScene = CreateRef<RenderScene>();

    QK_CORE_LOGI_TAG("Rernderer", "RenderSystem Initialized");
}

RenderSystem::~RenderSystem()
{
    
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
                RenderObject new_entity;

                // calculate entity's hash id
                util::Hasher hasher;
                hasher.u64(renderProxy.entity_id);
                hasher.u64(section_index);
                new_entity.id = hasher.get();

                new_entity.model_matrix = renderProxy.transform;
                new_entity.aabb = section_desc.aabb;
                new_entity.render_mesh_id = renderProxy.mesh_asset_id;
                new_entity.render_material_id = section_desc.material_asset_id;
                new_entity.index_count = section_desc.index_count;
                new_entity.start_index = section_desc.index_offset;

                // create render resources
                if (!m_renderResourceManager->IsMeshAssetRegisterd(renderProxy.mesh_asset_id)) 
                    m_renderResourceManager->CreateMeshRenderResouce(renderProxy.mesh_asset_id);

                if (section_desc.material_asset_id == 0)
                {
                    // use default material
                    new_entity.render_material_id = m_renderResourceManager->default_material_id;
                }
                else 
                {
                    // create material resource
                    if (!m_renderResourceManager->IsMaterialAssetRegisterd(section_desc.material_asset_id))
                        m_renderResourceManager->CreateMaterialRenderResource(section_desc.material_asset_id);
                }
                
                // add to render scene
                auto find = m_renderScene->render_object_to_offset.find(new_entity.id);
                if (find != m_renderScene->render_object_to_offset.end())
                {
                    m_renderScene->render_objects[find->second] = new_entity;
                }
                else
                {
                    m_renderScene->render_objects.push_back(new_entity);
                    m_renderScene->render_object_to_offset[new_entity.id] = m_renderScene->render_objects.size() - 1;
                    m_renderScene->render_object_to_entity[new_entity.id] = renderProxy.entity_id;
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

//void RenderSystem::UpdateDrawContext(const Ref<Scene>& scene, DrawContext& context)
//{
//    // update light data
//    context.sceneData.ambientColor = glm::vec4(.1f);
//    context.sceneData.sunlightColor = glm::vec4(1.f);
//    context.sceneData.sunlightDirection = glm::vec4(0, 1, 0.5, 1.f);
//
//    // Update render objects
//    context.objects_opaque.clear();
//    context.objects_transparent.clear();
//
//    const auto& cmpts = scene->GetComponents<IdCmpt, MeshCmpt, MeshRendererCmpt, TransformCmpt>();
//    for (const auto [id_cmpt, mesh_cmpt, mesh_renderer_cmpt, transform_cmpt] : cmpts)
//    {
//        auto* mesh = mesh_cmpt->uniqueMesh ? mesh_cmpt->uniqueMesh.get() : mesh_cmpt->sharedMesh.get();
//        if (!mesh) 
//            continue;
//
//        for (uint32_t i = 0; i < mesh->subMeshes.size(); ++i) {
//            const auto& submesh = mesh->subMeshes[i];
//
//            RenderObject newObj;
//            newObj.aabb = submesh.aabb;
//            newObj.firstIndex = submesh.startIndex;
//            newObj.indexCount = submesh.count;
//            newObj.indexBuffer = mesh->GetIndexBuffer();
//            newObj.attributeBuffer = mesh->GetAttributeBuffer();
//            newObj.positionBuffer = mesh->GetPositionBuffer();
//            newObj.material = mesh_renderer_cmpt->GetMaterial(i);
//            newObj.transform = transform_cmpt->GetWorldMatrix();
//            newObj.mainPassPipeLine = mesh_renderer_cmpt->GetGraphicsPipeLine(i);
//            newObj.entityID = id_cmpt->id;
//            if (newObj.material->alphaMode == AlphaMode::MODE_OPAQUE)
//                context.objects_opaque.push_back(newObj);
//            else
//                context.objects_transparent.push_back(newObj);
//        }
//    }
//}
//
//void RenderSystem::UpdateVisibility(const DrawContext& drawContext, Visibility& vis, const UniformBufferData_Camera& cameraData)
//{
//	vis.visible_opaque.clear();
//    vis.visible_transparent.clear();
//    vis.cameraData = cameraData;
//    vis.frustum.Build(glm::inverse(cameraData.viewproj));
//
//    auto is_visible = [&](const RenderObject& obj)
//    {
//        math::Aabb transformed_aabb = obj.aabb.Transform(obj.transform);
//        if (vis.frustum.CheckSphere(transformed_aabb))
//            return true;
//        else
//            return false;
//    };
//
//    for (size_t i = 0; i < drawContext.objects_opaque.size(); i++)
//    {
//        if (is_visible(drawContext.objects_opaque[i]))
//            vis.visible_opaque.push_back((uint32_t)i);
//    }
//
//    for (size_t i = 0; i < drawContext.objects_transparent.size(); i++)
//    {
//        if (is_visible(drawContext.objects_transparent[i]))
//            vis.visible_transparent.push_back((uint32_t)i);
//    }
//
//    // Sort render objects by material, pipleine and mesh
//    // The number of materials < the number of pipelines < the number of meshes
//    std::sort(vis.visible_opaque.begin(), vis.visible_opaque.end(), [&](const uint32_t& iA, const uint32_t& iB)
//    {
//        const RenderObject& A = drawContext.objects_opaque[iA];
//        const RenderObject& B = drawContext.objects_opaque[iB];
//        if (A.material == B.material)
//            return A.indexBuffer < B.indexBuffer;
//        else
//            return A.material < B.material;
//    });
//
//    std::sort(vis.visible_transparent.begin(), vis.visible_transparent.end(), [&](const uint32_t& iA, const uint32_t& iB)
//    {
//        const RenderObject& A = drawContext.objects_transparent[iA];
//        const RenderObject& B = drawContext.objects_transparent[iB];
//        if (A.material == B.material)
//            return A.indexBuffer < B.indexBuffer;
//        else
//            return A.material < B.material;
//    });
//}
//
//void RenderSystem::UpdateGpuResources(DrawContext& context, Visibility& vis)
//{
//    // create scene uniform buffer per frame
//	BufferDesc desc;
//	desc.domain = BufferMemoryDomain::CPU;
//	desc.size = sizeof(UniformBufferData_Scene);
//	desc.usageBits = BUFFER_USAGE_UNIFORM_BUFFER_BIT;
//	context.sceneUB = m_device->CreateBuffer(desc);
//    context.sceneData.cameraUboData = vis.cameraData;
//
//    UniformBufferData_Scene sceneData = context.sceneData;
//	sceneData.cameraUboData = vis.cameraData;
//
//    UniformBufferData_Scene* mappedData = (UniformBufferData_Scene*)context.sceneUB->GetMappedDataPtr();
//	*mappedData = sceneData;
//
//}

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
    Ref<rhi::PipeLine> pipeline_skybox = m_renderResourceManager->GetOrCreateGraphicsPSO(
        *m_renderResourceManager->GetShaderLibrary().staticProgram_skybox,
        cmd->GetCurrentRenderPassInfo(),
        m_renderResourceManager->mesh_attrib_mask_skybox,
        false, AlphaMode::MODE_OPAQUE);

    cmd->BindUniformBuffer(0, 0, *m_renderResourceManager->ubo_scene, 0, sizeof(UniformBufferData_Scene));

    cmd->BindImage(1, 0, *envMap, ImageLayout::SHADER_READ_ONLY_OPTIMAL);
    cmd->BindSampler(1, 0, *m_renderResourceManager->sampler_cube);
    cmd->BindVertexBuffer(0, *cubeRenderMesh.vertex_position_buffer, 0);
    cmd->BindIndexBuffer(*cubeRenderMesh.index_buffer, 0, IndexBufferFormat::UINT32);

    cmd->BindPipeLine(*pipeline_skybox);
    cmd->DrawIndexed((uint32_t)cubeMesh->indices.size(), 1, 0, 0, 0);
}

void RenderSystem::DrawGrid(rhi::CommandList* cmd)
{
    Ref<rhi::PipeLine> infiniteGrid_pipeline = m_renderResourceManager->GetOrCreateGraphicsPSO(
        *m_renderResourceManager->GetShaderLibrary().staticProgram_infiniteGrid,
        cmd->GetCurrentRenderPassInfo(), 0, true, AlphaMode::MODE_OPAQUE);
    
    cmd->BindUniformBuffer(0, 0, *m_renderResourceManager->ubo_scene, 0, sizeof(UniformBufferData_Scene));

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

    cmd->BindUniformBuffer(0, 0, *m_renderResourceManager->ubo_scene, 0, sizeof(UniformBufferData_Scene));
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
            cmd->BindVertexBuffer(2, *lastMesh->vertex_varying_buffer, 0);
			cmd->BindIndexBuffer(*lastMesh->index_buffer, 0, IndexBufferFormat::UINT32);
		}

        // push model constant
        PushConstants_Model push_constant;
        push_constant.worldMatrix = obj.model_matrix;

        cmd->PushConstant(&push_constant, 0, 64);  // only push model matrix

        // rebind Pipeline
        Ref<rhi::PipeLine> pipeline = m_renderResourceManager->GetOrCreateGraphicsPSO(
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
    
    cmd->BindUniformBuffer(0, 0, *m_renderResourceManager->ubo_scene, 0, sizeof(UniformBufferData_Scene));
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