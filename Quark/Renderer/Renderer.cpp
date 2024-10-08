#include "Quark/qkpch.h"
#include "Quark/Renderer/Renderer.h"
#include "Quark/Core/Application.h"
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
        depthStencilState_depthWrite.depthCompareOp = CompareOperation::LESS_OR_EQUAL;
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
        renderPassInfo2_simpleColorPass.numColorAttachments = 1;
        renderPassInfo2_simpleColorPass.colorAttachmentFormats[0] = format_colorAttachment_main;
        renderPassInfo2_simpleColorPass.sampleCount = SampleCount::SAMPLES_1;

        renderPassInfo2_simpleMainPass.numColorAttachments = 1;
        renderPassInfo2_simpleMainPass.colorAttachmentFormats[0] = format_colorAttachment_main;
        renderPassInfo2_simpleMainPass.depthAttachmentFormat = format_depthAttachment_main;
        renderPassInfo2_simpleMainPass.sampleCount = SampleCount::SAMPLES_1;

        renderPassInfo2_editorMainPass = renderPassInfo2_simpleMainPass;
        renderPassInfo2_editorMainPass.numColorAttachments = 2;
        renderPassInfo2_editorMainPass.colorAttachmentFormats[1] = DataFormat::R32G32_UINT;

        renderPassInfo2_uiPass.numColorAttachments = 1;
        renderPassInfo2_uiPass.colorAttachmentFormats[0] = Application::Get().GetGraphicDevice()->GetPresentImageFormat();
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
        pipelineDesc_skybox.renderPassInfo = renderPassInfo2_editorMainPass;
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
        ShaderProgramVariant* precompiledSkyboxVariant = GetShaderLibrary().staticProgram_skybox->GetPrecompiledVariant();

        pipeline_skybox = m_device->CreateGraphicPipeLine(pipelineDesc_skybox);
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

void Renderer::UpdatePerFrameData(const Ref<Scene>& scene, PerFrameData& perframeData)
{
    // update light data
    perframeData.sceneData.ambientColor = glm::vec4(.1f);
    perframeData.sceneData.sunlightColor = glm::vec4(1.f);
    perframeData.sceneData.sunlightDirection = glm::vec4(0, 1, 0.5, 1.f);

    // Update render objects
    perframeData.objects_opaque.clear();
    perframeData.objects_transparent.clear();

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
                perframeData.objects_opaque.push_back(newObj);
            else
                perframeData.objects_transparent.push_back(newObj);

            i++;
        }
    }
}

void Renderer::UpdateVisibility(const CameraUniformBufferData& cameraData, const PerFrameData& perframeData, Visibility& vis)
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

    for (size_t i = 0; i < perframeData.objects_opaque.size(); i++)
    {
        if (is_visible(perframeData.objects_opaque[i]))
            vis.visible_opaque.push_back(i);
    }

    for (size_t i = 0; i < perframeData.objects_transparent.size(); i++)
    {
        if (is_visible(perframeData.objects_transparent[i]))
            vis.visible_transparent.push_back(i);
    }

    // Sort render objects by material, pipleine and mesh
    // The number of materials < the number of pipelines < the number of meshes
    std::sort(vis.visible_opaque.begin(), vis.visible_opaque.end(), [&](const uint32_t& iA, const uint32_t& iB)
    {
        const RenderObject& A = perframeData.objects_opaque[iA];
        const RenderObject& B = perframeData.objects_opaque[iB];
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

    std::sort(vis.visible_transparent.begin(), vis.visible_transparent.end(), [&](const uint32_t& iA, const uint32_t& iB)
    {
        const RenderObject& A = perframeData.objects_transparent[iA];
        const RenderObject& B = perframeData.objects_transparent[iB];
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

void Renderer::UpdateGpuResources(PerFrameData& perframeData, Visibility& vis)
{
    // Create scene uniform buffer
	BufferDesc desc;
	desc.domain = BufferMemoryDomain::CPU;
	desc.size = sizeof(SceneUniformBufferData);
	desc.usageBits = BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	perframeData.sceneUB = m_device->CreateBuffer(desc);

	SceneUniformBufferData sceneData = perframeData.sceneData;
	sceneData.cameraUboData = vis.cameraData;
	SceneUniformBufferData* mappedData = (SceneUniformBufferData*)perframeData.sceneUB->GetMappedDataPtr();
	*mappedData = sceneData;

}

void Renderer::DrawSkybox(const PerFrameData& frame, const Ref<Texture>& envMap, graphic::CommandList* cmd)
{
    Ref<Mesh> cubeMesh = AssetManager::Get().mesh_cube;
    Ref<graphic::PipeLine> skyboxPipeLine = Renderer::Get().pipeline_skybox;

    cmd->BindPipeLine(*skyboxPipeLine);
    cmd->BindUniformBuffer(0, 0, *frame.sceneUB, 0, sizeof(SceneUniformBufferData));
    cmd->BindImage(0, 1, *envMap->image, ImageLayout::SHADER_READ_ONLY_OPTIMAL);
    cmd->BindSampler(0, 1, *envMap->sampler);
    cmd->BindVertexBuffer(0, *cubeMesh->GetPositionBuffer(), 0);
    cmd->BindIndexBuffer(*cubeMesh->GetIndexBuffer(), 0, IndexBufferFormat::UINT32);
    cmd->DrawIndexed((uint32_t)cubeMesh->indices.size(), 1, 0, 0, 0);
}

void Renderer::DrawScene(const PerFrameData& frame, const Visibility& vis, graphic::CommandList* cmd)
{

    Ref<Material> lastMaterial = nullptr;
    Ref<PipeLine> lastPipeline = nullptr;
    Ref<graphic::Buffer> lastIndexBuffer = nullptr;

    // Draw
    auto draw = [&](const RenderObject& obj)
    {
        // Bind Pipeline
        if (obj.pipeLine != lastPipeline)
        {
            lastPipeline = obj.pipeLine;
            cmd->BindPipeLine(*lastPipeline);

            // Bind scene uniform buffer
            cmd->BindUniformBuffer(0, 0, *frame.sceneUB, 0, sizeof(SceneUniformBufferData));
        }

        // Bind material
        if (obj.material != lastMaterial)
        {
            lastMaterial = obj.material;
            // (deprecated)cmd_list->BindUniformBuffer(1, 0, *lastMaterial->uniformBuffer, lastMaterial->uniformBufferOffset, sizeof(Material::UniformBufferBlock));
            cmd->BindImage(1, 1, *lastMaterial->baseColorTexture->image, ImageLayout::SHADER_READ_ONLY_OPTIMAL);
            cmd->BindSampler(1, 1, *lastMaterial->baseColorTexture->sampler);
            cmd->BindImage(1, 2, *lastMaterial->metallicRoughnessTexture->image, ImageLayout::SHADER_READ_ONLY_OPTIMAL);
            cmd->BindSampler(1, 2, *lastMaterial->metallicRoughnessTexture->sampler);

            MaterialPushConstants materialPushConstants;
            materialPushConstants.colorFactors = obj.material->uniformBufferData.baseColorFactor;
            materialPushConstants.metallicFactor = obj.material->uniformBufferData.metalicFactor;
            materialPushConstants.roughnessFactor = obj.material->uniformBufferData.roughNessFactor;
            cmd->PushConstant(&materialPushConstants, sizeof(glm::mat4), sizeof(MaterialPushConstants));
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
        ModelPushConstants push_constant;
        push_constant.worldMatrix = obj.transform;
        //push_constant.vertexBufferGpuAddress = obj.attributeBuffer->GetGpuAddress();
        cmd->PushConstant(&push_constant, 0, 64);  // only push model matrix
        cmd->PushConstant(&obj.entityID, 88, 8);

        cmd->DrawIndexed(obj.indexCount, 1, obj.firstIndex, 0, 0);
    };

    for (const uint32_t idx : vis.visible_opaque)
        draw(frame.objects_opaque[idx]);
}

Ref<graphic::PipeLine> Renderer::GetOrCreatePipeLine(
    const ShaderProgramVariant& programVariant,
    const graphic::PipelineDepthStencilState& ds,
    const graphic::PipelineColorBlendState& bs,
    const graphic::RasterizationState& rs,
    const graphic::RenderPassInfo2& rp,
    const graphic::VertexInputLayout& input)
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

    auto it = m_pipelines.find(hash);
    if (it != m_pipelines.end())
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

        Ref<graphic::PipeLine> newPipeline = Application::Get().GetGraphicDevice()->CreateGraphicPipeLine(desc);
        m_pipelines[hash] = newPipeline;

        return newPipeline;
    }
}

}