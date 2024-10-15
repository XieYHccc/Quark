#include "Quark/qkpch.h"
#include "Quark/Renderer/Renderer.h"
#include "Quark/Asset/AssetManager.h"
#include "Quark/Asset/Mesh.h"
#include "Quark/Graphic/Device.h"
#include "Quark/Scene/Scene.h"
#include "Quark/Scene/Components/CommonCmpts.h"
#include "Quark/Scene/Components/MeshCmpt.h"
#include "Quark/Scene/Components/TransformCmpt.h"
#include "Quark/Scene/Components/MeshRendererCmpt.h"

namespace quark {

using namespace graphic;

Renderer::Renderer(graphic::Device* device)
    : m_device(device)
{
    m_shaderLibrary = CreateScope<ShaderLibrary>();
    // m_sceneRenderer = CreateScope<SceneRenderer>(m_device);

    // depth stencil states
    {
        depthStencilState_disabled.enableDepthTest = false;
        depthStencilState_disabled.enableDepthWrite = false;

        depthStencilState_depthTestOnly.enableDepthTest = true;
        depthStencilState_depthTestOnly.enableDepthWrite = false;
        depthStencilState_depthTestOnly.depthCompareOp = CompareOperation::LESS_OR_EQUAL;

        depthStencilState_depthWrite.enableStencil = false;
        depthStencilState_depthWrite.enableDepthTest = true;
        depthStencilState_depthWrite.enableDepthWrite = true;
        depthStencilState_depthWrite.depthCompareOp = CompareOperation::LESS;
        depthStencilState_depthWrite.enableStencil = false;
    }

    // rasterization states
    {
        rasterizationState_fill.cullMode = CullMode::NONE;
        rasterizationState_fill.frontFaceType = FrontFaceType::COUNTER_CLOCKWISE;
        rasterizationState_fill.polygonMode = PolygonMode::Fill;
        rasterizationState_fill.enableDepthClamp = false;
        rasterizationState_fill.enableAntialiasedLine = false;
        rasterizationState_fill.forcedSampleCount = SampleCount::SAMPLES_1;
        rasterizationState_fill.lineWidth = 1.f;

        rasterizationState_wireframe.cullMode = CullMode::NONE;
        rasterizationState_wireframe.frontFaceType = FrontFaceType::COUNTER_CLOCKWISE;
        rasterizationState_wireframe.polygonMode = PolygonMode::Line;
        rasterizationState_wireframe.enableDepthClamp = false;
        rasterizationState_wireframe.enableAntialiasedLine = false;
        rasterizationState_wireframe.forcedSampleCount = SampleCount::SAMPLES_1;
        rasterizationState_wireframe.lineWidth = 1.5f;
    }

    // blend states
	{
        graphic::PipelineColorBlendState bs;
        bs.enable_independent_blend = false;
        bs.attachments[0].enable_blend = false;
        bs.attachments[0].colorWriteMask = util::ecast(ColorWriteFlagBits::ENABLE_ALL);
        blendState_opaque = bs;

        bs.attachments[0].enable_blend = true;
        bs.attachments[0].srcColorBlendFactor = BlendFactor::SRC_ALPHA;
        bs.attachments[0].dstColorBlendFactor = BlendFactor::ONE_MINUS_SRC_ALPHA;
        bs.attachments[0].colorBlendOp = BlendOperation::ADD;
        bs.attachments[0].srcAlphaBlendFactor = BlendFactor::ONE;
        bs.attachments[0].dstAlphaBlendFactor = BlendFactor::ONE_MINUS_SRC_ALPHA;
        bs.attachments[0].alphaBlendOp = BlendOperation::ADD;
        bs.attachments[0].colorWriteMask = util::ecast(ColorWriteFlagBits::ENABLE_ALL);
        bs.enable_independent_blend = false;
        blendState_transparent = bs;
	}

    // renderpass infos
    {
        renderPassInfo_swapchainPass.numColorAttachments = 1;
        renderPassInfo_swapchainPass.colorAttachmentFormats[0] = m_device->GetPresentImageFormat();
        renderPassInfo_swapchainPass.sampleCount = SampleCount::SAMPLES_1;

        renderPassInfo_simpleMainPass.numColorAttachments = 1;
        renderPassInfo_simpleMainPass.colorAttachmentFormats[0] = format_colorAttachment_main;
        renderPassInfo_simpleMainPass.depthAttachmentFormat = format_depthAttachment_main;
        renderPassInfo_simpleMainPass.sampleCount = SampleCount::SAMPLES_1;

        renderPassInfo_editorMainPass = renderPassInfo_simpleMainPass;
        renderPassInfo_editorMainPass.numColorAttachments = 2;
        renderPassInfo_editorMainPass.colorAttachmentFormats[1] = DataFormat::R32G32_UINT;

        renderPassInfo_entityIdPass.numColorAttachments = 1;
        renderPassInfo_entityIdPass.colorAttachmentFormats[0] = DataFormat::R32G32_UINT;
        renderPassInfo_entityIdPass.sampleCount = SampleCount::SAMPLES_1;
        renderPassInfo_entityIdPass.depthAttachmentFormat = format_depthAttachment_main;
    }

    // vertex input layout
    {
        VertexInputLayout::VertexBindInfo vert_bind_info;
        vert_bind_info.binding = 0; // position buffer
        vert_bind_info.stride = 12; // hardcoded stride, since we know cube mesh's attrib layout
        vert_bind_info.inputRate = VertexInputLayout::VertexBindInfo::INPUT_RATE_VERTEX;

        VertexInputLayout::VertexAttribInfo pos_attrib;
        pos_attrib.binding = 0;
        pos_attrib.format = VertexInputLayout::VertexAttribInfo::ATTRIB_FORMAT_VEC3;
        pos_attrib.location = 0;
        pos_attrib.offset = 0;

        vertexInputLayout_skybox.vertexBindInfos.push_back(vert_bind_info);
        vertexInputLayout_skybox.vertexAttribInfos.push_back(pos_attrib);
    }

    // pipeline descs
    {
        pipelineDesc_skybox.vertShader = GetShaderLibrary().staticProgram_skybox->GetPrecompiledVariant()->GetShader(ShaderStage::STAGE_VERTEX);
        pipelineDesc_skybox.fragShader = GetShaderLibrary().staticProgram_skybox->GetPrecompiledVariant()->GetShader(ShaderStage::STAGE_FRAGEMNT);
        pipelineDesc_skybox.blendState = blendState_opaque;
        pipelineDesc_skybox.depthStencilState = depthStencilState_disabled;
        pipelineDesc_skybox.rasterState = rasterizationState_fill;
        pipelineDesc_skybox.topologyType = TopologyType::TRANGLE_LIST;
        pipelineDesc_skybox.vertexInputLayout = vertexInputLayout_skybox;
        
        //TODO: remove hardcoded renderpass
        pipelineDesc_skybox.renderPassInfo = renderPassInfo_editorMainPass;

        pipelineDesc_infiniteGrid.vertShader = GetShaderLibrary().staticProgram_infiniteGrid->GetPrecompiledVariant()->GetShader(ShaderStage::STAGE_VERTEX);
        pipelineDesc_infiniteGrid.fragShader = GetShaderLibrary().staticProgram_infiniteGrid->GetPrecompiledVariant()->GetShader(ShaderStage::STAGE_FRAGEMNT);
        pipelineDesc_infiniteGrid.blendState = blendState_opaque;
        pipelineDesc_infiniteGrid.depthStencilState = depthStencilState_depthWrite;
        pipelineDesc_infiniteGrid.rasterState = rasterizationState_fill;
        pipelineDesc_infiniteGrid.topologyType = TopologyType::TRANGLE_LIST;

        pipelineDesc_infiniteGrid.renderPassInfo = renderPassInfo_editorMainPass;
    }

    // images
    {
        constexpr uint32_t white = 0xFFFFFFFF;
        constexpr uint32_t black = 0x000000FF;
        constexpr uint32_t magenta = 0xFF00FFFF;

        ImageDesc desc = {};
        desc.depth = 1;
        desc.format = DataFormat::R8G8B8A8_UNORM;
        desc.initialLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL;
        desc.type = ImageType::TYPE_2D;
        desc.usageBits = IMAGE_USAGE_CAN_COPY_TO_BIT | IMAGE_USAGE_SAMPLING_BIT;
        desc.arraySize = 1;
        desc.mipLevels = 1;
        desc.generateMipMaps = false;
        desc.width = 1;
        desc.height = 1;

        ImageInitData initData;
        initData.data = &white;
        initData.rowPitch = 4;
        initData.slicePitch = 1 * 1 * 4;

        image_white = device->CreateImage(desc, &initData);

        initData.data = &black;
        image_black = device->CreateImage(desc, &initData);

        // Checkboard image
        std::array<uint32_t, 32 * 32 > checkBoradpixelsData;
        for (int x = 0; x < 32; x++)
        {
            for (int y = 0; y < 32; y++)
                checkBoradpixelsData[y * 32 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
        }

        desc.width = 32;
        desc.height = 32;

        initData.data = checkBoradpixelsData.data();
        initData.rowPitch = 32 * 4;
        initData.slicePitch = 32 * 32 * 4;

        image_checkboard = device->CreateImage(desc, &initData);
    }

    // Samplers
    {
        SamplerDesc desc = {};
        desc.minFilter = SamplerFilter::LINEAR;
        desc.magFliter = SamplerFilter::LINEAR;
        desc.addressModeU = SamplerAddressMode::REPEAT;
        desc.addressModeV = SamplerAddressMode::REPEAT;
        desc.addressModeW = SamplerAddressMode::REPEAT;

        sampler_linear = device->CreateSampler(desc);

        desc.minFilter = SamplerFilter::NEAREST;
        desc.magFliter = SamplerFilter::NEAREST;

        sampler_nearst = device->CreateSampler(desc);

        // Cubemap sampler
        desc.minFilter = SamplerFilter::LINEAR;
        desc.magFliter = SamplerFilter::LINEAR;
        desc.addressModeU = SamplerAddressMode::CLAMPED_TO_EDGE;
        desc.addressModeV = SamplerAddressMode::CLAMPED_TO_EDGE;
        desc.addressModeW = SamplerAddressMode::CLAMPED_TO_EDGE;

        sampler_cube = device->CreateSampler(desc);
    }

    // Pipelines
    {
        pipeline_skybox = m_device->CreateGraphicPipeLine(pipelineDesc_skybox);
        pipeline_infiniteGrid = m_device->CreateGraphicPipeLine(pipelineDesc_infiniteGrid);

        // entityId pipeline
        graphic::GraphicPipeLineDesc desc;
        desc.vertShader = GetShaderLibrary().staticProgram_entityID->GetPrecompiledVariant()->GetShader(ShaderStage::STAGE_VERTEX);
        desc.fragShader = GetShaderLibrary().staticProgram_entityID->GetPrecompiledVariant()->GetShader(ShaderStage::STAGE_FRAGEMNT);
        desc.depthStencilState = depthStencilState_depthWrite;
        desc.blendState = blendState_opaque;
        desc.rasterState = rasterizationState_fill;
        desc.topologyType = TopologyType::TRANGLE_LIST;
        desc.renderPassInfo = renderPassInfo_entityIdPass;
        desc.vertexInputLayout = vertexInputLayout_skybox;
        pipeline_entityID = m_device->CreateGraphicPipeLine(desc);
    }

    QK_CORE_LOGI_TAG("Rernderer", "Renderer Initialized");
}

Renderer::~Renderer()
{
    // Free gpu resources
    image_white.reset();
    image_black.reset();
    image_checkboard.reset();
    
    sampler_linear.reset();
    sampler_nearst.reset();
    sampler_cube.reset();

    m_shaderLibrary.reset();
}

void Renderer::UpdateDrawContext(const Ref<Scene>& scene, DrawContext& context)
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
            newObj.pipeLine = mesh_renderer_cmpt->GetGraphicsPipeLine(i);
            newObj.entityID = id_cmpt->id;
            if (newObj.material->alphaMode == AlphaMode::MODE_OPAQUE)
                context.objects_opaque.push_back(newObj);
            else
                context.objects_transparent.push_back(newObj);

            i++;
        }
    }
}

void Renderer::UpdateVisibility(const DrawContext& drawContext, Visibility& vis, const UniformBufferData_Camera& cameraData)
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

void Renderer::UpdateGpuResources(DrawContext& DrawContext, Visibility& vis)
{
    // Create scene uniform buffer
	BufferDesc desc;
	desc.domain = BufferMemoryDomain::CPU;
	desc.size = sizeof(UniformBufferData_Scene);
	desc.usageBits = BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	DrawContext.sceneUB = m_device->CreateBuffer(desc);

    UniformBufferData_Scene sceneData = DrawContext.sceneData;
	sceneData.cameraUboData = vis.cameraData;
    UniformBufferData_Scene* mappedData = (UniformBufferData_Scene*)DrawContext.sceneUB->GetMappedDataPtr();
	*mappedData = sceneData;
}

void Renderer::DrawSkybox(const DrawContext& context, const Ref<Texture>& envMap, graphic::CommandList* cmd)
{
    Ref<Mesh> cubeMesh = AssetManager::Get().mesh_cube;
    Ref<graphic::PipeLine> skybox_pipeline = GetGraphicsPipeline(*m_shaderLibrary->staticProgram_skybox, {}, cmd->GetCurrentRenderPassInfo(), vertexInputLayout_skybox, false, AlphaMode::MODE_OPAQUE);
    
    cmd->BindUniformBuffer(0, 0, *context.sceneUB, 0, sizeof(UniformBufferData_Scene));

    cmd->BindPipeLine(*skybox_pipeline);
    cmd->BindImage(1, 0, *envMap->image, ImageLayout::SHADER_READ_ONLY_OPTIMAL);
    cmd->BindSampler(1, 0, *envMap->sampler);
    cmd->BindVertexBuffer(0, *cubeMesh->GetPositionBuffer(), 0);
    cmd->BindIndexBuffer(*cubeMesh->GetIndexBuffer(), 0, IndexBufferFormat::UINT32);
    cmd->DrawIndexed((uint32_t)cubeMesh->indices.size(), 1, 0, 0, 0);
}

void Renderer::DrawGrid(const DrawContext& context, graphic::CommandList* cmd)
{
    Ref<graphic::PipeLine> infiniteGrid_pipeline = GetGraphicsPipeline(*m_shaderLibrary->staticProgram_infiniteGrid, {}, cmd->GetCurrentRenderPassInfo(), {}, true, AlphaMode::MODE_OPAQUE);
    
    cmd->BindUniformBuffer(0, 0, *context.sceneUB, 0, sizeof(UniformBufferData_Scene));

    cmd->BindPipeLine(*infiniteGrid_pipeline);
    cmd->Draw(6, 1, 0, 0);

}
void Renderer::DrawEntityID(const DrawContext& context, const Visibility& vis, graphic::CommandList* cmd)
{
    QK_CORE_ASSERT(cmd->GetCurrentRenderPassInfo().colorAttachmentFormats[0] == graphic::DataFormat::R32G32_UINT)
    
    cmd->BindUniformBuffer(0, 0, *context.sceneUB, 0, sizeof(UniformBufferData_Scene));
    cmd->BindPipeLine(*pipeline_entityID);

    for (const uint32_t id : vis.visible_opaque)
    {
        const RenderObject& obj = context.objects_opaque[id];
        cmd->BindVertexBuffer(0, *obj.positionBuffer, 0);
        cmd->BindVertexBuffer(1, *obj.attributeBuffer, 0);
        cmd->BindIndexBuffer(*obj.indexBuffer, 0, IndexBufferFormat::UINT32);
        cmd->PushConstant(&obj.entityID, 64, 8);

        cmd->DrawIndexed(obj.indexCount, 1, obj.firstIndex, 0, 0);
    }
}

void Renderer::DrawScene(const DrawContext& context, const Visibility& vis, graphic::CommandList* cmd)
{
    Ref<Material> lastMaterial = nullptr;
    Ref<PipeLine> lastPipeline = nullptr;
    Ref<graphic::Buffer> lastIndexBuffer = nullptr;

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
        if (obj.pipeLine != lastPipeline)
        {
            lastPipeline = obj.pipeLine;
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
        cmd->PushConstant(&obj.entityID, 88, 8);

        cmd->DrawIndexed(obj.indexCount, 1, obj.firstIndex, 0, 0);
    };

    for (const uint32_t idx : vis.visible_opaque)
        draw(context.objects_opaque[idx]);
}

Ref<graphic::PipeLine> Renderer::GetGraphicsPipeline(const ShaderProgramVariant& programVariant, const graphic::PipelineDepthStencilState& ds, const graphic::PipelineColorBlendState& bs, const graphic::RasterizationState& rs, const graphic::RenderPassInfo2& rp, const graphic::VertexInputLayout& input)
{
    util::Hasher h;

    // hash depth stencil state
    h.u32(static_cast<uint32_t>(ds.enableDepthTest));
    h.u32(static_cast<uint32_t>(ds.enableDepthWrite));
    h.u32(util::ecast(ds.depthCompareOp));
    h.u32(util::ecast(rs.polygonMode));
    h.u32(util::ecast(rs.cullMode));
    h.u32(util::ecast(rs.frontFaceType));
    h.u64(programVariant.GetHash());

    // hash blend state
    h.u32(uint32_t(bs.enable_independent_blend));

    for (uint32_t i = 0; i < rp.numColorAttachments; i++)
    {
        size_t attachmentIndex = 0;
        if (bs.enable_independent_blend)
            attachmentIndex = i;

        const auto& att = bs.attachments[attachmentIndex];

        h.u32(static_cast<uint32_t>(att.enable_blend));
        if (att.enable_blend)
        {
            h.u32(util::ecast(att.colorBlendOp));
            h.u32(util::ecast(att.srcColorBlendFactor));
            h.u32(util::ecast(att.dstColorBlendFactor));
            h.u32(util::ecast(att.alphaBlendOp));
            h.u32(util::ecast(att.srcAlphaBlendFactor));
            h.u32(util::ecast(att.dstAlphaBlendFactor));
        }
    }

    // hash render pass info
    h.u64(rp.GetHash());

    // hash vertex input layout
    for (const auto& attrib : input.vertexAttribInfos)
    {
        h.u32(util::ecast(attrib.format));
        h.u32(attrib.offset);
        h.u32(attrib.binding);
    }

    for (const auto& b : input.vertexBindInfos)
    {
        h.u32(b.binding);
        h.u32(b.stride);
        h.u32(util::ecast(b.inputRate));
    }

    uint64_t hash = h.get();

    auto it = m_cached_pipelines.find(hash);
    if (it != m_cached_pipelines.end())
    {
        return it->second;
    }
    else
    {
        graphic::GraphicPipeLineDesc desc = {};
        desc.vertShader = programVariant.GetShader(graphic::ShaderStage::STAGE_VERTEX);
        desc.fragShader = programVariant.GetShader(graphic::ShaderStage::STAGE_FRAGEMNT);
        desc.depthStencilState = ds;
        desc.blendState = bs;
        desc.rasterState = rs;
        desc.topologyType = graphic::TopologyType::TRANGLE_LIST;
        desc.renderPassInfo = rp;
        desc.vertexInputLayout = input;

        Ref<graphic::PipeLine> newPipeline = m_device->CreateGraphicPipeLine(desc);
        m_cached_pipelines[hash] = newPipeline;

        return newPipeline;
    }
}

Ref<graphic::PipeLine> Renderer::GetGraphicsPipeline(ShaderProgram& program, const ShaderVariantKey& key, const graphic::RenderPassInfo2& rp, const graphic::VertexInputLayout& vertexLayout, bool enableDepth, AlphaMode mode)
{
    ShaderProgramVariant* programVariant = program.IsStatic()? program.GetPrecompiledVariant() : program.GetOrCreateVariant(key);
    graphic::PipelineColorBlendState& bs = mode == AlphaMode::MODE_OPAQUE ? blendState_opaque : blendState_transparent;
    graphic::PipelineDepthStencilState ds = depthStencilState_disabled;
    if (enableDepth)
        ds = mode == AlphaMode::MODE_OPAQUE ? depthStencilState_depthWrite : depthStencilState_depthTestOnly;

    util::Hasher h;
    h.u64(programVariant->GetHash());
    h.u64(rp.GetHash());

    // hash depth stencil state
    h.u32(static_cast<uint32_t>(ds.enableDepthTest));
    h.u32(static_cast<uint32_t>(ds.enableDepthWrite));
    h.u32(util::ecast(ds.depthCompareOp));

    // hash blend state
    h.u32(uint32_t(bs.enable_independent_blend));
    for (uint32_t i = 0; i < rp.numColorAttachments; i++)
    {
        size_t attachmentIndex = 0;
        if (bs.enable_independent_blend)
            attachmentIndex = i;

        const auto& att = bs.attachments[attachmentIndex];

        h.u32(static_cast<uint32_t>(att.enable_blend));
        if (att.enable_blend)
        {
            h.u32(util::ecast(att.colorBlendOp));
            h.u32(util::ecast(att.srcColorBlendFactor));
            h.u32(util::ecast(att.dstColorBlendFactor));
            h.u32(util::ecast(att.alphaBlendOp));
            h.u32(util::ecast(att.srcAlphaBlendFactor));
            h.u32(util::ecast(att.dstAlphaBlendFactor));
        }
    }

    // hash vertex input layout
    for (const auto& attrib : vertexLayout.vertexAttribInfos)
    {
        h.u32(util::ecast(attrib.format));
        h.u32(attrib.offset);
        h.u32(attrib.binding);
    }

    for (const auto& b : vertexLayout.vertexBindInfos)
    {
        h.u32(b.binding);
        h.u32(b.stride);
        h.u32(util::ecast(b.inputRate));
    }

    util::Hash hash = h.get();
    auto it = m_cached_pipelines.find(hash);
    if (it != m_cached_pipelines.end())
    {
        return it->second;
    }
    else
    {
        graphic::GraphicPipeLineDesc desc = {};
        desc.vertShader = programVariant->GetShader(graphic::ShaderStage::STAGE_VERTEX);
        desc.fragShader = programVariant->GetShader(graphic::ShaderStage::STAGE_FRAGEMNT);
        desc.depthStencilState = ds;
        desc.blendState = bs;
        desc.rasterState = rasterizationState_fill;
        desc.topologyType = graphic::TopologyType::TRANGLE_LIST;
        desc.renderPassInfo = rp;
        if (vertexLayout.isValid())
            desc.vertexInputLayout = vertexLayout;

        Ref<graphic::PipeLine> newPipeline = m_device->CreateGraphicPipeLine(desc);
        m_cached_pipelines[hash] = newPipeline;

        return newPipeline;
    }
}

Ref<graphic::VertexInputLayout> Renderer::GetVertexInputLayout(uint32_t meshAttributesMask)
{
    auto it = m_cached_vertexInputLayouts.find(meshAttributesMask);
    if (it != m_cached_vertexInputLayouts.end())
		return it->second;

    Ref<graphic::VertexInputLayout> newLayout = CreateRef<graphic::VertexInputLayout>();

    // Position
    if (meshAttributesMask & MESH_ATTRIBUTE_POSITION_BIT)
    {
        graphic::VertexInputLayout::VertexAttribInfo& attrib = newLayout->vertexAttribInfos.emplace_back();
        attrib.location = 0;
        attrib.binding = 0;
        attrib.format = graphic::VertexInputLayout::VertexAttribInfo::ATTRIB_FORMAT_VEC3;
        attrib.offset = 0;
    }

    uint32_t offset = 0;

    // UV
    if (meshAttributesMask & MESH_ATTRIBUTE_UV_BIT)
    {
        graphic::VertexInputLayout::VertexAttribInfo& attrib = newLayout->vertexAttribInfos.emplace_back();
        attrib.location = 1;
        attrib.binding = 1;
        attrib.format = graphic::VertexInputLayout::VertexAttribInfo::ATTRIB_FORMAT_VEC2;
        attrib.offset = offset;
        offset += sizeof(glm::vec2);
    }

    // Normal
    if (meshAttributesMask & MESH_ATTRIBUTE_NORMAL_BIT)
    {
        graphic::VertexInputLayout::VertexAttribInfo& attrib = newLayout->vertexAttribInfos.emplace_back();
        attrib.location = 2;
        attrib.binding = 1;
        attrib.format = graphic::VertexInputLayout::VertexAttribInfo::ATTRIB_FORMAT_VEC3;
        attrib.offset = offset;
        offset += sizeof(glm::vec3);
    }

    // Vertex Color
    if (meshAttributesMask & MESH_ATTRIBUTE_VERTEX_COLOR_BIT)
    {
        graphic::VertexInputLayout::VertexAttribInfo& attrib = newLayout->vertexAttribInfos.emplace_back();
        attrib.location = 3;
        attrib.binding = 1;
        attrib.format = graphic::VertexInputLayout::VertexAttribInfo::ATTRIB_FORMAT_VEC4;
        attrib.offset = offset;
        offset += sizeof(glm::vec4);
    }

    graphic::VertexInputLayout::VertexBindInfo bindInfo = {};
    bindInfo.binding = 0;
    bindInfo.stride = sizeof(glm::vec3);
    bindInfo.inputRate = graphic::VertexInputLayout::VertexBindInfo::INPUT_RATE_VERTEX;
    newLayout->vertexBindInfos.push_back(bindInfo);

    bindInfo.binding = 1;
    bindInfo.stride = offset;
    bindInfo.inputRate = graphic::VertexInputLayout::VertexBindInfo::INPUT_RATE_VERTEX;
    newLayout->vertexBindInfos.push_back(bindInfo);

    m_cached_vertexInputLayouts[meshAttributesMask] = newLayout;
    return newLayout;
}

}