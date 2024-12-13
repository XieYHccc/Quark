#include "Quark/qkpch.h"
#include "Quark/Render/RenderSystem.h"
#include "Quark/Asset/AssetManager.h"
#include "Quark/Asset/Mesh.h"
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
                RenderObject1 new_entity;

                // calculate entity's hash id
                util::Hasher hasher;
                hasher.u64(renderProxy.entity_id);
                hasher.u64(section_index);
                new_entity.id = hasher.get();

                new_entity.model_matrix = renderProxy.transform;
                new_entity.bounding_box = section_desc.aabb;
                new_entity.render_mesh_id = renderProxy.mesh_asset_id;
                new_entity.render_material_id = section_desc.material_asset_id;

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
        renderSwapData.camera_swap_data.reset();
    }

    m_renderResourceManager->UpdatePerFrameBuffer(m_renderScene);

}

void RenderSystem::UpdateDrawContext(const Ref<Scene>& scene, DrawContext& context)
{
    // update light data
    context.sceneData.ambientColor = glm::vec4(.1f);
    context.sceneData.sunlightColor = glm::vec4(1.f);
    context.sceneData.sunlightDirection = glm::vec4(0, 1, 0.5, 1.f);

    // Update render objects
    context.objects_opaque.clear();
    context.objects_transparent.clear();

    const auto& cmpts = scene->GetComponents<IdCmpt, MeshCmpt, MeshRendererCmpt, TransformCmpt>();
    for (const auto [id_cmpt, mesh_cmpt, mesh_renderer_cmpt, transform_cmpt] : cmpts)
    {
        auto* mesh = mesh_cmpt->uniqueMesh ? mesh_cmpt->uniqueMesh.get() : mesh_cmpt->sharedMesh.get();
        if (!mesh) 
            continue;

        for (uint32_t i = 0; i < mesh->subMeshes.size(); ++i) {
            const auto& submesh = mesh->subMeshes[i];

            RenderObject newObj;
            newObj.aabb = submesh.aabb;
            newObj.firstIndex = submesh.startIndex;
            newObj.indexCount = submesh.count;
            newObj.indexBuffer = mesh->GetIndexBuffer();
            newObj.attributeBuffer = mesh->GetAttributeBuffer();
            newObj.positionBuffer = mesh->GetPositionBuffer();
            newObj.material = mesh_renderer_cmpt->GetMaterial(i);
            newObj.transform = transform_cmpt->GetWorldMatrix();
            newObj.mainPassPipeLine = mesh_renderer_cmpt->GetGraphicsPipeLine(i);
            newObj.entityID = id_cmpt->id;
            if (newObj.material->alphaMode == AlphaMode::MODE_OPAQUE)
                context.objects_opaque.push_back(newObj);
            else
                context.objects_transparent.push_back(newObj);
        }
    }
}

void RenderSystem::UpdateVisibility(const DrawContext& drawContext, Visibility& vis, const UniformBufferData_Camera& cameraData)
{
	vis.visible_opaque.clear();
    vis.visible_transparent.clear();
    vis.cameraData = cameraData;
    vis.frustum.Build(glm::inverse(cameraData.viewproj));

    auto is_visible = [&](const RenderObject& obj)
    {
        math::Aabb transformed_aabb = obj.aabb.Transform(obj.transform);
        if (vis.frustum.CheckSphere(transformed_aabb))
            return true;
        else
            return false;
    };

    for (size_t i = 0; i < drawContext.objects_opaque.size(); i++)
    {
        if (is_visible(drawContext.objects_opaque[i]))
            vis.visible_opaque.push_back((uint32_t)i);
    }

    for (size_t i = 0; i < drawContext.objects_transparent.size(); i++)
    {
        if (is_visible(drawContext.objects_transparent[i]))
            vis.visible_transparent.push_back((uint32_t)i);
    }

    // Sort render objects by material, pipleine and mesh
    // The number of materials < the number of pipelines < the number of meshes
    std::sort(vis.visible_opaque.begin(), vis.visible_opaque.end(), [&](const uint32_t& iA, const uint32_t& iB)
    {
        const RenderObject& A = drawContext.objects_opaque[iA];
        const RenderObject& B = drawContext.objects_opaque[iB];
        if (A.material == B.material)
            return A.indexBuffer < B.indexBuffer;
        else
            return A.material < B.material;
    });

    std::sort(vis.visible_transparent.begin(), vis.visible_transparent.end(), [&](const uint32_t& iA, const uint32_t& iB)
    {
        const RenderObject& A = drawContext.objects_transparent[iA];
        const RenderObject& B = drawContext.objects_transparent[iB];
        if (A.material == B.material)
            return A.indexBuffer < B.indexBuffer;
        else
            return A.material < B.material;
    });
}

void RenderSystem::UpdateGpuResources(DrawContext& context, Visibility& vis)
{
    // create scene uniform buffer per frame
	BufferDesc desc;
	desc.domain = BufferMemoryDomain::CPU;
	desc.size = sizeof(UniformBufferData_Scene);
	desc.usageBits = BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	context.sceneUB = m_device->CreateBuffer(desc);
    context.sceneData.cameraUboData = vis.cameraData;

    UniformBufferData_Scene sceneData = context.sceneData;
	sceneData.cameraUboData = vis.cameraData;

    UniformBufferData_Scene* mappedData = (UniformBufferData_Scene*)context.sceneUB->GetMappedDataPtr();
	*mappedData = sceneData;

}
void RenderSystem::DrawSkybox(uint64_t env_map_id, rhi::CommandList* cmd)
{
    Ref<Mesh> cubeMesh = AssetManager::Get().mesh_cube;
    // TODO: Remove this after change asset manager
    if (!m_renderResourceManager->IsMeshAssetRegisterd(cubeMesh->GetAssetID()))
        m_renderResourceManager->CreateMeshRenderResouce(cubeMesh->GetAssetID());
    
    if (!m_renderResourceManager->IsImageAssetRegisterd(env_map_id))
        m_renderResourceManager->CreateImageRenderResource(env_map_id);

    auto& cubeRenderMesh = m_renderResourceManager->GetRenderMesh(cubeMesh->GetAssetID());
    Ref<rhi::Image> envMap = m_renderResourceManager->GetImage(env_map_id);
    Ref<rhi::PipeLine> pipeline_skybox = m_renderResourceManager->GetOrCreateGraphicsPipeline(*m_renderResourceManager->GetShaderLibrary().staticProgram_skybox, {}, cmd->GetCurrentRenderPassInfo(), m_renderResourceManager->vertexInputLayout_skybox, false, AlphaMode::MODE_OPAQUE);

    cmd->BindUniformBuffer(0, 0, *m_renderResourceManager->ubo_scene, 0, sizeof(UniformBufferData_Scene));

    cmd->BindImage(1, 0, *envMap, ImageLayout::SHADER_READ_ONLY_OPTIMAL);
    cmd->BindSampler(1, 0, *m_renderResourceManager->sampler_cube);
    cmd->BindVertexBuffer(0, *cubeMesh->GetPositionBuffer(), 0);
    cmd->BindIndexBuffer(*cubeMesh->GetIndexBuffer(), 0, IndexBufferFormat::UINT32);

    cmd->BindPipeLine(*pipeline_skybox);
    cmd->DrawIndexed((uint32_t)cubeMesh->indices.size(), 1, 0, 0, 0);
}

void RenderSystem::DrawSkybox(const DrawContext& context, const Ref<Texture>& envMap, rhi::CommandList* cmd)
{
    Ref<Mesh> cubeMesh = AssetManager::Get().mesh_cube;
    Ref<rhi::PipeLine> pipeline_skybox = m_renderResourceManager->GetOrCreateGraphicsPipeline(*m_renderResourceManager->GetShaderLibrary().staticProgram_skybox, {}, cmd->GetCurrentRenderPassInfo(), m_renderResourceManager->vertexInputLayout_skybox, false, AlphaMode::MODE_OPAQUE);
    
    cmd->BindUniformBuffer(0, 0, *context.sceneUB, 0, sizeof(UniformBufferData_Scene));

    cmd->BindImage(1, 0, *envMap->image, ImageLayout::SHADER_READ_ONLY_OPTIMAL);
    cmd->BindSampler(1, 0, *envMap->sampler);
    cmd->BindVertexBuffer(0, *cubeMesh->GetPositionBuffer(), 0);
    cmd->BindIndexBuffer(*cubeMesh->GetIndexBuffer(), 0, IndexBufferFormat::UINT32);

    cmd->BindPipeLine(*pipeline_skybox);
    cmd->DrawIndexed((uint32_t)cubeMesh->indices.size(), 1, 0, 0, 0);
}

void RenderSystem::DrawGrid(const DrawContext& context, rhi::CommandList* cmd)
{
    Ref<rhi::PipeLine> infiniteGrid_pipeline = m_renderResourceManager->GetOrCreateGraphicsPipeline(*m_renderResourceManager->GetShaderLibrary().staticProgram_infiniteGrid, {}, cmd->GetCurrentRenderPassInfo(), {}, true, AlphaMode::MODE_OPAQUE);
    
    cmd->BindUniformBuffer(0, 0, *context.sceneUB, 0, sizeof(UniformBufferData_Scene));

    cmd->BindPipeLine(*infiniteGrid_pipeline);
    cmd->Draw(6, 1, 0, 0);

}

void RenderSystem::DrawEntityID(const DrawContext& context, const Visibility& vis, rhi::CommandList* cmd)
{
    QK_CORE_ASSERT(cmd->GetCurrentRenderPassInfo().colorAttachmentFormats[0] == rhi::DataFormat::R32G32_UINT)
    
    cmd->BindUniformBuffer(0, 0, *context.sceneUB, 0, sizeof(UniformBufferData_Scene));
    cmd->BindPipeLine(*m_renderResourceManager->pipeline_entityID);

    for (const uint32_t id : vis.visible_opaque)
    {
        const RenderObject& obj = context.objects_opaque[id];
        cmd->BindVertexBuffer(0, *obj.positionBuffer, 0);
        cmd->BindIndexBuffer(*obj.indexBuffer, 0, IndexBufferFormat::UINT32);
        cmd->PushConstant(&obj.transform, 0, 64);
        cmd->PushConstant(&obj.entityID, 64, 8);

        cmd->DrawIndexed(obj.indexCount, 1, obj.firstIndex, 0, 0);
    }
}

void RenderSystem::DrawScene(const DrawContext& context, const Visibility& vis, rhi::CommandList* cmd)
{
    Ref<Material> lastMaterial = nullptr;
    Ref<PipeLine> lastPipeline = nullptr;
    Ref<rhi::Buffer> lastIndexBuffer = nullptr;

    // Bind scene uniform buffer(assume all pipeline are using the same pipeline layout)
    cmd->BindUniformBuffer(0, 0, *context.sceneUB, 0, sizeof(UniformBufferData_Scene));

    // Draw
    auto draw = [&](const RenderObject& obj)
    {
        // rebind material
        if (obj.material != lastMaterial)
        {
            lastMaterial = obj.material;
            // (deprecated)cmd_list->BindUniformBuffer(1, 0, *lastMaterial->uniformBuffer, lastMaterial->uniformBufferOffset, sizeof(Material::UniformBufferBlock));
            cmd->BindImage(1, 1, *lastMaterial->baseColorTexture->image, ImageLayout::SHADER_READ_ONLY_OPTIMAL);
            cmd->BindSampler(1, 1, *lastMaterial->baseColorTexture->sampler);
            cmd->BindImage(1, 2, *lastMaterial->metallicRoughnessTexture->image, ImageLayout::SHADER_READ_ONLY_OPTIMAL);
            cmd->BindSampler(1, 2, *lastMaterial->metallicRoughnessTexture->sampler);

            PushConstants_Material materialPushConstants;
            materialPushConstants.colorFactors = obj.material->uniformBufferData.baseColorFactor;
            materialPushConstants.metallicFactor = obj.material->uniformBufferData.metalicFactor;
            materialPushConstants.roughnessFactor = obj.material->uniformBufferData.roughNessFactor;
            cmd->PushConstant(&materialPushConstants, sizeof(glm::mat4), sizeof(PushConstants_Material));
        }

        // rebind Pipeline
        if (obj.mainPassPipeLine != lastPipeline)
        {
            lastPipeline = obj.mainPassPipeLine;
            cmd->BindPipeLine(*lastPipeline);
        }

        // Bind index buffer
        if (obj.indexBuffer != lastIndexBuffer)
        {
            cmd->BindVertexBuffer(0, *obj.positionBuffer, 0);
            cmd->BindVertexBuffer(1, *obj.attributeBuffer, 0);
            cmd->BindIndexBuffer(*obj.indexBuffer, 0, IndexBufferFormat::UINT32);
            lastIndexBuffer = obj.indexBuffer;
        }

        // Push model constant
        PushConstants_Model push_constant;
        push_constant.worldMatrix = obj.transform;
        //push_constant.vertexBufferGpuAddress = obj.attributeBuffer->GetGpuAddress();

        cmd->PushConstant(&push_constant, 0, 64);  // only push model matrix
        cmd->DrawIndexed(obj.indexCount, 1, obj.firstIndex, 0, 0);
    };

    for (const uint32_t idx : vis.visible_opaque)
        draw(context.objects_opaque[idx]);
}

}