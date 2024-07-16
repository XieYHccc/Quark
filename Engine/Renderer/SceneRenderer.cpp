#include "pch.h"
#include "Renderer/SceneRenderer.h"
#include "Scene/Scene.h"
#include "Scene/Components/MeshCmpt.h"
#include "Scene/Components/TransformCmpt.h"
#include "Scene/Components/CameraCmpt.h"
#include "Graphic/Device.h"

namespace render {

void SceneRenderer::PrepareForRender()
{
    // prepare light information
    drawContext_.sceneData.ambientColor = glm::vec4(.1f);
	drawContext_.sceneData.sunlightColor = glm::vec4(1.f);
	drawContext_.sceneData.sunlightDirection = glm::vec4(0,1,0.5,1.f);
    
    UpdateDrawContext();
}

void SceneRenderer::UpdateDrawContext()
{
    // Clear render objects
    drawContext_.opaqueObjects.clear();
    drawContext_.transparentObjects.clear();

    // Fill render objects
    const auto& mesh_transform_cmpts = scene_->GetComponents<scene::MeshCmpt, scene::TransformCmpt>();
    for (const auto [mesh_cmpt, transform_cmpt] : mesh_transform_cmpts) {
        auto mesh = mesh_cmpt->mesh;
        for (const auto& submesh : mesh->subMeshes) {
            RenderObject new_renderObject;
            new_renderObject.aabb = submesh.aabb;
            new_renderObject.firstIndex = submesh.startIndex;
            new_renderObject.indexCount = submesh.count;
            new_renderObject.indexBuffer = mesh->indexBuffer.get();
            new_renderObject.vertexBuffer = mesh->vertexBuffer.get();
            new_renderObject.material = submesh.material.get();
            new_renderObject.transform = transform_cmpt->GetWorldMatrix();

            if (new_renderObject.material->alphaMode == Material::AlphaMode::OPAQUE) {
                drawContext_.opaqueObjects.push_back(new_renderObject);
            }
            else {
                drawContext_.transparentObjects.push_back(new_renderObject);
            }
        }
    }

    // Update scene uniform buffer
    auto* mainCam = scene_->GetCamera();
    
    CORE_DEBUG_ASSERT(mainCam);
	SceneUniformBufferBlock& sceneData = drawContext_.sceneData;
	sceneData.view = mainCam->GetViewMatrix();
	sceneData.proj = mainCam->GetProjectionMatrix();
	sceneData.proj[1][1] *= -1;
	sceneData.viewproj = sceneData.proj * sceneData.view;

    // Update frustum
    frustum_.build(glm::inverse(sceneData.viewproj));
}

void SceneRenderer::Render(graphic::CommandList* cmd_list)
{
    CORE_DEBUG_ASSERT(device_)
    using namespace graphic;

    // Update draw context
    UpdateDrawContext();

    std::vector<u32>& opaque_draws = drawContext_.opaqueDraws;
    std::vector<u32>& transparent_draws = drawContext_.transparentDraws;
    opaque_draws.clear();
    opaque_draws.reserve(drawContext_.opaqueObjects.size());
    transparent_draws.clear();
    transparent_draws.reserve(drawContext_.transparentObjects.size());

    // Frustum culling
    auto is_visible = [&](const RenderObject& obj) {
        math::Aabb transformed_aabb = obj.aabb.transform(obj.transform);
        if (frustum_.check_shpere(transformed_aabb))
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

    // allocate a new uniform buffer for the scene data
    BufferDesc scene_buffer_desc = {
        .domain = BufferMemoryDomain::CPU,
        .size = sizeof(SceneUniformBufferBlock),
        .usageBits = BUFFER_USAGE_UNIFORM_BUFFER_BIT
    };
    Ref<Buffer> scene_uniform_buffer = device_->CreateBuffer(scene_buffer_desc);

    SceneUniformBufferBlock* mapped_data = (SceneUniformBufferBlock*)scene_uniform_buffer->GetMappedDataPtr();
    *mapped_data = drawContext_.sceneData;

    // Bind scene uniform buffer
    cmd_list->BindUniformBuffer(0, 0, *scene_uniform_buffer, 0, sizeof(SceneUniformBufferBlock));

    render::Material* last_mat = nullptr;
    graphic::Buffer* last_indexBuffer = nullptr;
    auto draw = [&] (const RenderObject& obj) {

        // Bind material
        if (obj.material != last_mat) {
            last_mat = obj.material;
            cmd_list->BindUniformBuffer(1, 0, *last_mat->uniformBuffer, last_mat->uniformBufferOffset, sizeof(render::Material::UniformBufferBlock));
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