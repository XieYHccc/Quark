#include "Quark/qkpch.h"
#include "Quark/Renderer/SceneRenderer.h"
#include "Quark/Renderer/Renderer.h"
#include "Quark/Scene/Scene.h"
#include "Quark/Scene/Components/MeshCmpt.h"
#include "Quark/Scene/Components/TransformCmpt.h"
#include "Quark/Scene/Components/CameraCmpt.h"
#include "Quark/Scene/Components/MeshRendererCmpt.h"
#include "Quark/Graphic/Device.h"
#include "Quark/Asset/MeshImporter.h"

namespace quark {

using namespace graphic;
SceneRenderer::SceneRenderer(graphic::Device* device)
    : m_GraphicDevice(device)
{
    // Load Cube mesh
    MeshImporter mesh_loader;
    m_CubeMesh = mesh_loader.ImportGLTF("BuiltInResources/Gltf/cube.gltf");

    // Create scene uniform buffer
    BufferDesc desc;
    desc.domain = BufferMemoryDomain::CPU;
    desc.size = sizeof(SceneUniformBufferBlock);
    desc.usageBits = BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    m_DrawContext.sceneUniformBuffer = m_GraphicDevice->CreateBuffer(desc);

    // prepare light information
    m_DrawContext.sceneUboData.ambientColor = glm::vec4(.1f);
    m_DrawContext.sceneUboData.sunlightColor = glm::vec4(1.f);
    m_DrawContext.sceneUboData.sunlightDirection = glm::vec4(0, 1, 0.5, 1.f);
}

void SceneRenderer::UpdateRenderObjects()
{
    // Clear render objects
    m_DrawContext.opaqueObjects.clear();
    m_DrawContext.transparentObjects.clear();

    // Fill render objects
    const auto& cmpts = m_Scene->GetComponents<MeshCmpt, MeshRendererCmpt,TransformCmpt>();
    for (const auto [mesh_cmpt, mesh_renderer_cmpt, transform_cmpt] : cmpts) {
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
            if (new_renderObject.material->alphaMode == AlphaMode::OPAQUE)
                m_DrawContext.opaqueObjects.push_back(new_renderObject);
            else
                m_DrawContext.transparentObjects.push_back(new_renderObject);

            i++;
        }

    }
}

void SceneRenderer::UpdateDrawContext()
{
    // Update RenderObjects
    UpdateRenderObjects();

    // Update scene uniform buffer
    auto* mainCameraEntity = m_Scene->GetMainCameraEntity();

    if (mainCameraEntity)
    {
        auto* cameraCmpt = mainCameraEntity->GetComponent<CameraCmpt>();

        CameraUniformBufferBlock& cameraData = m_DrawContext.sceneUboData.cameraUboData;
        cameraData.view = cameraCmpt->GetViewMatrix();
        cameraData.proj = cameraCmpt->GetProjectionMatrix();
        cameraData.proj[1][1] *= -1;
        cameraData.viewproj = cameraData.proj * cameraData.view;

        // Update frustum
        m_DrawContext.frustum.Build(glm::inverse(cameraData.viewproj));
    }

    SceneUniformBufferBlock* mapped_data = (SceneUniformBufferBlock*)m_DrawContext.sceneUniformBuffer->GetMappedDataPtr();
    *mapped_data = m_DrawContext.sceneUboData;
}

void SceneRenderer::UpdateDrawContext(const CameraUniformBufferBlock& cameraData)
{
    // Update RenderObjects
    UpdateRenderObjects();

    // Update camera data
    m_DrawContext.sceneUboData.cameraUboData = cameraData;
    m_DrawContext.frustum.Build(glm::inverse(cameraData.viewproj));

    // Update Scene uniform buffer
    SceneUniformBufferBlock* mapped_data = (SceneUniformBufferBlock*)m_DrawContext.sceneUniformBuffer->GetMappedDataPtr();
    *mapped_data = m_DrawContext.sceneUboData;

}

void SceneRenderer::RenderSkybox(graphic::CommandList *cmd_list)
{
    QK_CORE_ASSERT(m_CubeMap)

    Ref<graphic::PipeLine> skyboxPipeLine = Renderer::Get().pipeline_skybox;
    cmd_list->BindPipeLine(*skyboxPipeLine);
    cmd_list->BindUniformBuffer(0, 0, *m_DrawContext.sceneUniformBuffer, 0, sizeof(SceneUniformBufferBlock));
    cmd_list->BindImage(0, 1, *m_CubeMap->image, ImageLayout::SHADER_READ_ONLY_OPTIMAL);
    cmd_list->BindSampler(0, 1, *m_CubeMap->sampler);
    cmd_list->BindVertexBuffer(0, *m_CubeMesh->GetPositionBuffer(), 0);
    cmd_list->BindIndexBuffer(*m_CubeMesh->GetIndexBuffer(), 0, IndexBufferFormat::UINT32);
    cmd_list->DrawIndexed((uint32_t)m_CubeMesh->indices.size(), 1, 0, 0, 0);
}

void SceneRenderer::RenderScene(graphic::CommandList* cmd_list)
{
    std::vector<u32>& opaque_draws = m_DrawContext.opaqueDraws;
    std::vector<u32>& transparent_draws = m_DrawContext.transparentDraws;
    opaque_draws.clear();
    opaque_draws.reserve(m_DrawContext.opaqueObjects.size());
    transparent_draws.clear();
    transparent_draws.reserve(m_DrawContext.transparentObjects.size());

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
            opaque_draws.push_back(i);
    }

    // Sort render objects by material, pipleine and mesh
    // The number of materials > the number of pipelines > the number of meshes
    std::sort(opaque_draws.begin(), opaque_draws.end(), [&](const u32& iA, const u32& iB) 
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
        push_constant.vertexBufferGpuAddress = obj.attributeBuffer->GetGpuAddress();
        cmd_list->PushConstant(&push_constant, 0, 64);  // only push model matrix

        cmd_list->DrawIndexed(obj.indexCount, 1, obj.firstIndex, 0, 0);
    };

    for (const u32& idx : opaque_draws)
        draw(m_DrawContext.opaqueObjects[idx]);

    // TODO: Draw transparent objects

}

}