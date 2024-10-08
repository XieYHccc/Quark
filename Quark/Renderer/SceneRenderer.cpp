#include "Quark/qkpch.h"
#include "Quark/Renderer/SceneRenderer.h"
#include "Quark/Renderer/Renderer.h"
#include "Quark/Scene/Scene.h"
#include "Quark/Scene/Components/CommonCmpts.h"
#include "Quark/Scene/Components/MeshCmpt.h"
#include "Quark/Scene/Components/TransformCmpt.h"
#include "Quark/Scene/Components/CameraCmpt.h"
#include "Quark/Scene/Components/MeshRendererCmpt.h"
#include "Quark/Asset/AssetManager.h"

namespace quark {

using namespace graphic;
SceneRenderer::SceneRenderer(graphic::Device* device)
    : m_GraphicDevice(device)
{

    // Create scene uniform buffer
    BufferDesc desc;
    desc.domain = BufferMemoryDomain::CPU;
    desc.size = sizeof(SceneUniformBufferBlock);
    desc.usageBits = BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    m_DrawContext.sceneUniformBuffer = m_GraphicDevice->CreateBuffer(desc);

}

void SceneRenderer::UpdateRenderObjects()
{
    // Clear render objects
    m_DrawContext.opaqueObjects.clear();
    m_DrawContext.transparentObjects.clear();

    // Fill render objects
    const auto& cmpts = m_Scene->GetComponents<IdCmpt, MeshCmpt, MeshRendererCmpt, TransformCmpt>();
    for (const auto [id_cmpt, mesh_cmpt, mesh_renderer_cmpt, transform_cmpt] : cmpts) 
    {
        auto* mesh = mesh_cmpt->uniqueMesh ? mesh_cmpt->uniqueMesh.get() : mesh_cmpt->sharedMesh.get();

        if (!mesh) continue;

        for (uint32_t i = 0; const auto & submesh : mesh->subMeshes) {
            RenderObject new_renderObject;
            new_renderObject.aabb = submesh.aabb;
            new_renderObject.firstIndex = submesh.startIndex;
            new_renderObject.indexCount = submesh.count;
            new_renderObject.indexBuffer = mesh->GetIndexBuffer();
            new_renderObject.attributeBuffer = mesh->GetAttributeBuffer();
            new_renderObject.positionBuffer = mesh->GetPositionBuffer();
            new_renderObject.material = mesh_renderer_cmpt->GetMaterial(i);
            new_renderObject.transform = transform_cmpt->GetWorldMatrix();
            new_renderObject.pipeLine = mesh_renderer_cmpt->GetGraphicsPipeLine(i);
            new_renderObject.entityID = id_cmpt->id;
            if (new_renderObject.material->alphaMode == AlphaMode::MODE_OPAQUE)
                m_DrawContext.opaqueObjects.push_back(new_renderObject);
            else
                m_DrawContext.transparentObjects.push_back(new_renderObject);

            i++;
        }

    }

    m_DrawContext.opaqueDraws.clear();
    m_DrawContext.transparentDraws.clear();

    // Frustum culling
    auto is_visible = [&](const RenderObject& obj)
    {
        math::Aabb transformed_aabb = obj.aabb.Transform(obj.transform);
        if (m_DrawContext.frustum.CheckSphere(transformed_aabb))
            return true;
        else
            return false;
    };

    for (u32 i = 0; i < m_DrawContext.opaqueObjects.size(); i++)
    {
        if (is_visible(m_DrawContext.opaqueObjects[i]))
            m_DrawContext.opaqueDraws.push_back(i);
    }

    // Sort render objects by material, pipleine and mesh
    // The number of materials > the number of pipelines > the number of meshes
    std::sort(m_DrawContext.opaqueDraws.begin(), m_DrawContext.opaqueDraws.end(), [&](const u32& iA, const u32& iB)
    {
        const RenderObject& A = m_DrawContext.opaqueObjects[iA];
        const RenderObject& B = m_DrawContext.opaqueObjects[iB];
        if (A.material == B.material)
        {
            if (A.pipeLine == B.pipeLine)
                return A.indexBuffer < B.indexBuffer;
            else
                return A.pipeLine < B.pipeLine;
        }
        else
            return A.material < B.material;
    });
}

void SceneRenderer::UpdateDrawContext()
{
    // Update scene uniform buffer
    auto* mainCameraEntity = m_Scene->GetMainCameraEntity();

    if (!mainCameraEntity)
        return;

    auto* cameraCmpt = mainCameraEntity->GetComponent<CameraCmpt>();

    SceneUniformBufferBlock sceneUboData;
    CameraUniformBufferBlock cameraUboData;
    cameraUboData.view = cameraCmpt->GetViewMatrix();
    cameraUboData.proj = cameraCmpt->GetProjectionMatrix();
    cameraUboData.proj[1][1] *= -1;
    cameraUboData.viewproj = cameraUboData.proj * cameraUboData.view;
    sceneUboData.cameraUboData = cameraUboData;

    // prepare light information
    sceneUboData.ambientColor = glm::vec4(.1f);
    sceneUboData.sunlightColor = glm::vec4(1.f);
    sceneUboData.sunlightDirection = glm::vec4(0, 1, 0.5, 1.f);

    // Update Scene Uniform Buffer
    SceneUniformBufferBlock* mapped_data = (SceneUniformBufferBlock*)m_DrawContext.sceneUniformBuffer->GetMappedDataPtr();
    *mapped_data = sceneUboData;

    // Update frustum
    m_DrawContext.frustum.Build(glm::inverse(cameraUboData.viewproj));

    // Update RenderObjects
    UpdateRenderObjects();
}

void SceneRenderer::UpdateDrawContextEditor(const CameraUniformBufferBlock& cameraData)
{
    // Update Scene uniform buffer
    SceneUniformBufferBlock sceneUboData;
    sceneUboData.cameraUboData = cameraData;
    sceneUboData.ambientColor = glm::vec4(.1f);
    sceneUboData.sunlightColor = glm::vec4(1.f);
    sceneUboData.sunlightDirection = glm::vec4(0, 1, 0.5, 1.f);

    SceneUniformBufferBlock* mapped_data = (SceneUniformBufferBlock*)m_DrawContext.sceneUniformBuffer->GetMappedDataPtr();
    *mapped_data = sceneUboData;

    // Update Frustum
    m_DrawContext.frustum.Build(glm::inverse(cameraData.viewproj));

    // Update RenderObjects
    UpdateRenderObjects();
}

void SceneRenderer::DrawSkybox(const Ref<Texture>& envMap, graphic::CommandList *cmd_list)
{
    Ref<Mesh> cubeMesh = AssetManager::Get().mesh_cube;
    Ref<graphic::PipeLine> skyboxPipeLine = Renderer::Get().pipeline_skybox;

    cmd_list->BindPipeLine(*skyboxPipeLine);
    cmd_list->BindUniformBuffer(0, 0, *m_DrawContext.sceneUniformBuffer, 0, sizeof(SceneUniformBufferBlock));
    cmd_list->BindImage(0, 1, *envMap->image, ImageLayout::SHADER_READ_ONLY_OPTIMAL);
    cmd_list->BindSampler(0, 1, *envMap->sampler);
    cmd_list->BindVertexBuffer(0, *cubeMesh->GetPositionBuffer(), 0);
    cmd_list->BindIndexBuffer(*cubeMesh->GetIndexBuffer(), 0, IndexBufferFormat::UINT32);
    cmd_list->DrawIndexed((uint32_t)cubeMesh->indices.size(), 1, 0, 0, 0);
}

void SceneRenderer::DrawScene(graphic::CommandList* cmd_list)
{
    Ref<Material> lastMaterial = nullptr;
    Ref<PipeLine> lastPipeline = nullptr;
    Ref<graphic::Buffer> lastIndexBuffer = nullptr;

    auto draw = [&] (const RenderObject& obj) 
    {
        // Bind Pipeline
        if (obj.pipeLine != lastPipeline)
        {
            lastPipeline = obj.pipeLine;
            cmd_list->BindPipeLine(*lastPipeline);

            // Bind scene uniform buffer
            cmd_list->BindUniformBuffer(0, 0, *m_DrawContext.sceneUniformBuffer, 0, sizeof(SceneUniformBufferBlock));
        }

        // Bind material
        if (obj.material != lastMaterial)
        {
            lastMaterial = obj.material;
            // (deprecated)cmd_list->BindUniformBuffer(1, 0, *lastMaterial->uniformBuffer, lastMaterial->uniformBufferOffset, sizeof(Material::UniformBufferBlock));
            cmd_list->BindImage(1, 1, *lastMaterial->baseColorTexture->image, ImageLayout::SHADER_READ_ONLY_OPTIMAL);
            cmd_list->BindSampler(1, 1, *lastMaterial->baseColorTexture->sampler);
            cmd_list->BindImage(1, 2, *lastMaterial->metallicRoughnessTexture->image, ImageLayout::SHADER_READ_ONLY_OPTIMAL);
            cmd_list->BindSampler(1, 2, *lastMaterial->metallicRoughnessTexture->sampler);

            MaterialPushConstants materialPushConstants;
            materialPushConstants.colorFactors = obj.material->uniformBufferData.baseColorFactor;
            materialPushConstants.metallicFactor = obj.material->uniformBufferData.metalicFactor;
            materialPushConstants.roughnessFactor = obj.material->uniformBufferData.roughNessFactor;
            cmd_list->PushConstant(&materialPushConstants, sizeof(glm::mat4), sizeof(MaterialPushConstants));
        }
        
        // Bind index buffer
        if (obj.indexBuffer != lastIndexBuffer) 
        {
            cmd_list->BindVertexBuffer(0, *obj.positionBuffer, 0);
            cmd_list->BindVertexBuffer(1, *obj.attributeBuffer, 0);
            cmd_list->BindIndexBuffer(*obj.indexBuffer, 0, IndexBufferFormat::UINT32);
            lastIndexBuffer = obj.indexBuffer;
        }

        // Push model constant
        ModelPushConstants push_constant;
        push_constant.worldMatrix = obj.transform;
        //push_constant.vertexBufferGpuAddress = obj.attributeBuffer->GetGpuAddress();
        cmd_list->PushConstant(&push_constant, 0, 64);  // only push model matrix
        cmd_list->PushConstant(&obj.entityID, 88, 8);

        cmd_list->DrawIndexed(obj.indexCount, 1, obj.firstIndex, 0, 0);
    };

    for (const uint32_t idx : m_DrawContext.opaqueDraws)
        draw(m_DrawContext.opaqueObjects[idx]);

    // TODO: Draw transparent objects

}

}