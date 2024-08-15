#include "Quark/QuarkPch.h"
#include "Quark/Renderer/SceneRenderer.h"
#include "Quark/Scene/Scene.h"
#include "Quark/Scene/Components/MeshCmpt.h"
#include "Quark/Scene/Components/TransformCmpt.h"
#include "Quark/Scene/Components/CameraCmpt.h"
#include "Quark/Graphic/Device.h"
#include "Quark/Asset/MeshLoader.h"

namespace quark {

using namespace graphic;
SceneRenderer::SceneRenderer(graphic::Device* device)
    : device_(device)
{
    // Load Cube mesh
    MeshLoader mesh_loader(device_);
    cubeMesh_ = mesh_loader.LoadGLTF("Assets/Gltf/cube.gltf");

    // Create cube map sampler
    SamplerDesc sampler_desc;
    sampler_desc.minFilter = SamplerFilter::LINEAR;
    sampler_desc.magFliter = SamplerFilter::LINEAR;
    sampler_desc.addressModeU = SamplerAddressMode::CLAMPED_TO_EDGE;
    sampler_desc.addressModeU = SamplerAddressMode::CLAMPED_TO_EDGE;
    sampler_desc.addressModeU = SamplerAddressMode::CLAMPED_TO_EDGE;
    cubeMapSampler_ = device_->CreateSampler(sampler_desc);

    // Create scene uniform buffer
    BufferDesc scene_buffer_desc;
    scene_buffer_desc.domain = BufferMemoryDomain::CPU;
    scene_buffer_desc.size = sizeof(SceneUniformBufferBlock);
    scene_buffer_desc.usageBits = BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    drawContext_.sceneUniformBuffer = device_->CreateBuffer(scene_buffer_desc);
}

void SceneRenderer::PrepareForRender()
{
    // prepare light information
    drawContext_.sceneData.ambientColor = glm::vec4(.1f);
	drawContext_.sceneData.sunlightColor = glm::vec4(1.f);
	drawContext_.sceneData.sunlightDirection = glm::vec4(0,1,0.5,1.f);
    
}

void SceneRenderer::SetScene(Scene* scene)
{
    scene_ = scene;
    PrepareForRender();
}

void SceneRenderer::UpdateDrawContext()
{
    // Clear render objects
    drawContext_.opaqueObjects.clear();
    drawContext_.transparentObjects.clear();

    // Fill render objects
    const auto& mesh_transform_cmpts = scene_->GetComponents<MeshCmpt, TransformCmpt>();
    for (const auto [mesh_cmpt, transform_cmpt] : mesh_transform_cmpts) {
        auto* mesh = mesh_cmpt->uniqueMesh? mesh_cmpt->uniqueMesh.get() : mesh_cmpt->sharedMesh.get();

        if (!mesh) continue;
        for (const auto& submesh : mesh->subMeshes) {
            RenderObject new_renderObject;
            new_renderObject.aabb = submesh.aabb;
            new_renderObject.firstIndex = submesh.startIndex;
            new_renderObject.indexCount = submesh.count;
            new_renderObject.indexBuffer = mesh->indexBuffer.get();
            new_renderObject.vertexBuffer = mesh->vertexBuffer.get();
            new_renderObject.material = submesh.material.get();
            new_renderObject.transform = transform_cmpt->GetWorldMatrix();

            if (new_renderObject.material->alphaMode == AlphaMode::OPAQUE) 
                drawContext_.opaqueObjects.push_back(new_renderObject);
            else
                drawContext_.transparentObjects.push_back(new_renderObject);
        }
    }

    // Update scene uniform buffer
    auto* mainCameraEntity = scene_->GetMainCameraEntity();

    if (mainCameraEntity)
    {
        auto* cameraCmpt = mainCameraEntity->GetComponent<CameraCmpt>();

        SceneUniformBufferBlock& sceneData = drawContext_.sceneData;
        sceneData.view = cameraCmpt->GetViewMatrix();
        sceneData.proj = cameraCmpt->GetProjectionMatrix();
        sceneData.proj[1][1] *= -1;
        sceneData.viewproj = sceneData.proj * sceneData.view;

        SceneUniformBufferBlock* mapped_data = (SceneUniformBufferBlock*)drawContext_.sceneUniformBuffer->GetMappedDataPtr();
        *mapped_data = drawContext_.sceneData;

        // Update frustum
        drawContext_.frustum_.Build(glm::inverse(sceneData.viewproj));
    }

}

void SceneRenderer::RenderSkybox(graphic::CommandList *cmd_list)
{
    CORE_DEBUG_ASSERT(cubeMap_)

    cmd_list->BindUniformBuffer(0, 0, *drawContext_.sceneUniformBuffer, 0, sizeof(SceneUniformBufferBlock));
    cmd_list->BindImage(0, 1, *cubeMap_, ImageLayout::SHADER_READ_ONLY_OPTIMAL);
    cmd_list->BindSampler(0, 1, *cubeMapSampler_);
    cmd_list->BindVertexBuffer(0, *cubeMesh_->vertexBuffer, 0);
    cmd_list->BindIndexBuffer(*cubeMesh_->indexBuffer, 0, IndexBufferFormat::UINT32);
    cmd_list->DrawIndexed(cubeMesh_->indices.size(), 1, 0, 0, 0);
}

void SceneRenderer::RenderScene(graphic::CommandList* cmd_list)
{
    CORE_DEBUG_ASSERT(device_)
    
    std::vector<u32>& opaque_draws = drawContext_.opaqueDraws;
    std::vector<u32>& transparent_draws = drawContext_.transparentDraws;
    opaque_draws.clear();
    opaque_draws.reserve(drawContext_.opaqueObjects.size());
    transparent_draws.clear();
    transparent_draws.reserve(drawContext_.transparentObjects.size());

    // Frustum culling
    auto is_visible = [&](const RenderObject& obj) {
        math::Aabb transformed_aabb = obj.aabb.Transform(obj.transform);
        if (drawContext_.frustum_.CheckSphere(transformed_aabb))
            return true;
        else
            return false;
    };

    for (u32 i = 0; i < drawContext_.opaqueObjects.size(); i++) {
        if (is_visible(drawContext_.opaqueObjects[i])) {
            opaque_draws.push_back(i);
        }
        // opaque_draws.push_back(i);
    }

    // Sort render objects by material and mesh
    std::sort(opaque_draws.begin(), opaque_draws.end(), [&](const u32& iA, const u32& iB) {
        const RenderObject& A = drawContext_.opaqueObjects[iA];
        const RenderObject& B = drawContext_.opaqueObjects[iB];
        if (A.material == B.material)
            return A.indexBuffer < B.indexBuffer;
        else
            return A.material < B.material;
    });

    // Bind scene uniform buffer
    cmd_list->BindUniformBuffer(0, 0, *drawContext_.sceneUniformBuffer, 0, sizeof(SceneUniformBufferBlock));

    Material* last_mat = nullptr;
    graphic::Buffer* last_indexBuffer = nullptr;
    auto draw = [&] (const RenderObject& obj) {

        // Bind material
        if (obj.material != last_mat) {
            last_mat = obj.material;
            cmd_list->BindUniformBuffer(1, 0, *last_mat->uniformBuffer, last_mat->uniformBufferOffset, sizeof(Material::UniformBufferBlock));
            cmd_list->BindImage(1, 1, *last_mat->baseColorTexture->image, ImageLayout::SHADER_READ_ONLY_OPTIMAL);
            cmd_list->BindSampler(1, 1, *last_mat->baseColorTexture->sampler);
            cmd_list->BindImage(1, 2, *last_mat->metallicRoughnessTexture->image, ImageLayout::SHADER_READ_ONLY_OPTIMAL);
            cmd_list->BindSampler(1, 2, *last_mat->metallicRoughnessTexture->sampler);
        }
        
        // Bind index buffer
        if (obj.indexBuffer != last_indexBuffer) {
            cmd_list->BindIndexBuffer(*obj.indexBuffer, 0, IndexBufferFormat::UINT32);
            last_indexBuffer = obj.indexBuffer;
        }

        // Bind push constant
        GpuDrawPushConstants push_constant = {
            .worldMatrix = obj.transform,
            .vertexBufferGpuAddress = obj.vertexBuffer->GetGpuAddress(),
        };
        cmd_list->BindPushConstant(&push_constant, 0, sizeof(GpuDrawPushConstants));

        cmd_list->DrawIndexed(obj.indexCount, 1, obj.firstIndex, 0, 0);
    };

    for (const u32& idx : opaque_draws) {
        draw(drawContext_.opaqueObjects[idx]);
    }

    // TODO: Draw transparent objects

}

}