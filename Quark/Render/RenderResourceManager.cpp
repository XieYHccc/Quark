#include "Quark/qkpch.h"
#include "Quark/Render/RenderResourceManger.h"
#include "Quark/Asset/AssetManager.h"
#include "Quark/RHI/Device.h"

namespace quark
{

RenderResourceManager::RenderResourceManager(Ref<rhi::Device> device)
    : m_device(device)
{
    using namespace rhi;

    QK_CORE_VERIFY(device);
    m_shader_library = CreateScope<ShaderLibrary>();

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
        rhi::PipelineColorBlendState bs;
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

    // pipeline descs
    {
        //pipelineDesc_skybox.vertShader = GetShaderLibrary().staticProgram_skybox->GetPrecompiledVariant()->GetShader(ShaderStage::STAGE_VERTEX);
        //pipelineDesc_skybox.fragShader = GetShaderLibrary().staticProgram_skybox->GetPrecompiledVariant()->GetShader(ShaderStage::STAGE_FRAGEMNT);
        //pipelineDesc_skybox.blendState = blendState_opaque;
        //pipelineDesc_skybox.depthStencilState = depthStencilState_disabled;
        //pipelineDesc_skybox.rasterState = rasterizationState_fill;
        //pipelineDesc_skybox.topologyType = TopologyType::TRANGLE_LIST;
        //pipelineDesc_skybox.vertexInputLayout = vertexInputLayout_skybox;
        //
        ////TODO: remove hardcoded renderpass
        //pipelineDesc_skybox.renderPassInfo = renderPassInfo_editorMainPass;

        //pipelineDesc_infiniteGrid.vertShader = GetShaderLibrary().staticProgram_infiniteGrid->GetPrecompiledVariant()->GetShader(ShaderStage::STAGE_VERTEX);
        //pipelineDesc_infiniteGrid.fragShader = GetShaderLibrary().staticProgram_infiniteGrid->GetPrecompiledVariant()->GetShader(ShaderStage::STAGE_FRAGEMNT);
        //pipelineDesc_infiniteGrid.blendState = blendState_opaque;
        //pipelineDesc_infiniteGrid.depthStencilState = depthStencilState_depthWrite;
        //pipelineDesc_infiniteGrid.rasterState = rasterizationState_fill;
        //pipelineDesc_infiniteGrid.topologyType = TopologyType::TRANGLE_LIST;

        //pipelineDesc_infiniteGrid.renderPassInfo = renderPassInfo_editorMainPass;
    }

    // vertex input layout
    {
        mesh_attrib_mask_skybox = MESH_ATTRIBUTE_POSITION_BIT;

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
        //pipeline_skybox = m_device->CreateGraphicPipeLine(pipelineDesc_skybox);
        //pipeline_infiniteGrid = m_device->CreateGraphicPipeLine(pipelineDesc_infiniteGrid);

        // entityId pipeline
        rhi::GraphicPipeLineDesc desc;
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

    // default material
    {
        default_material = CreateRef<PBRMaterial>();
        default_material->textures[util::ecast(TextureKind::Albedo)] = image_black;
        default_material->textures[util::ecast(TextureKind::MetallicRoughness)] = image_white;
        default_material->base_color = glm::vec4(1.f);
        default_material->metallic_factor = 1.f;
        default_material->roughness_factor = 1.f;
        default_material->draw_pipeline = DrawPipeline::Opaque;
        default_material->shader_program = GetShaderLibrary().program_staticMesh;
        default_material->hash = 1; // TODO: any better way?
    }
}

std::vector<Ref<IRenderable>> RenderResourceManager::RequestStaticMeshRenderables(Ref<MeshAsset> mesh_asset)
{
    if (!mesh_asset)
		return {};

    auto find = m_static_meshes.find(mesh_asset->GetAssetID());
    if (find != m_static_meshes.end())
		return find->second;

    Ref<MeshBuffers> mesh_buffers = RequestMeshBuffers(mesh_asset);
    QK_CORE_ASSERT(mesh_buffers);

    std::vector<Ref<IRenderable>> renderables;
	for (auto& submesh : mesh_asset->subMeshes)
	{
		Ref<StaticMesh> renderable = CreateRef<StaticMesh>();
        renderable->vertex_count = submesh.count;
        renderable->vertex_offset = 0;
        renderable->ibo_offset = submesh.startIndex;
        renderable->mesh_buffers = mesh_buffers;
        renderable->mesh_attribute_mask = mesh_asset->GetMeshAttributeMask();
        renderable->static_aabb = submesh.aabb;
        Ref<PBRMaterial> mat = RequestMateral(AssetManager::Get().GetAsset<MaterialAsset>(submesh.materialID));
        renderable->material = mat ? mat : default_material;
        util::Hasher h;
        h.u64(mesh_asset->GetAssetID());
        h.u32(renderable->ibo_offset);
        h.u32(renderable->vertex_offset);
        h.u32(renderable->vertex_count);

        renderable->hash = h.get();
        renderables.push_back(renderable);

        m_renderables[h.get()] = renderable;
	}
        
    m_static_meshes[mesh_asset->GetAssetID()] = renderables;

	return renderables;
}

Ref<MeshBuffers> RenderResourceManager::RequestMeshBuffers(Ref<MeshAsset> mesh_asset)
{
    QK_CORE_ASSERT(mesh_asset);

    auto find = m_mesh_buffers.find(mesh_asset->GetAssetID());
    if (find != m_mesh_buffers.end())
        return find->second;

    Ref<MeshBuffers> new_mesh_buffers = CreateRef<MeshBuffers>();

    // upload index buffer
    uint32_t index_buffer_size = sizeof(uint32_t) * mesh_asset->indices.size();
    uint32_t* index_buffer_data = mesh_asset->indices.data();

    rhi::BufferDesc index_buffer_desc;
    index_buffer_desc.size = index_buffer_size;
    index_buffer_desc.usageBits = rhi::BUFFER_USAGE_INDEX_BUFFER_BIT | rhi::BUFFER_USAGE_TRANSFER_TO_BIT;
    index_buffer_desc.domain = rhi::BufferMemoryDomain::GPU;
    new_mesh_buffers->ibo = m_device->CreateBuffer(index_buffer_desc, index_buffer_data);

    // prepare staging buffer
    uint64_t vertex_position_buffer_size = sizeof(glm::vec3) * mesh_asset->vertex_positions.size();
    uint64_t vertex_varying_enable_blending_buffer_size = 0;
    uint64_t vertex_varying_buffer_size = 0;
    uint64_t vertex_joint_binding_buffer_size = 0;

    if (!mesh_asset->vertex_normals.empty())
        vertex_varying_enable_blending_buffer_size += sizeof(glm::vec3) * mesh_asset->vertex_normals.size();
    if (!mesh_asset->vertex_tangents.empty())
        vertex_varying_enable_blending_buffer_size += sizeof(glm::vec3) * mesh_asset->vertex_tangents.size();
    if (!mesh_asset->vertex_uvs.empty())
        vertex_varying_buffer_size += sizeof(glm::vec2) * mesh_asset->vertex_uvs.size();
    if (!mesh_asset->vertex_colors.empty())
        vertex_varying_buffer_size += sizeof(glm::vec4) * mesh_asset->vertex_colors.size();
    if (!mesh_asset->vertex_bone_indices.empty())
        vertex_joint_binding_buffer_size += sizeof(glm::uvec4) * mesh_asset->vertex_bone_indices.size();
    if (!mesh_asset->vertex_bone_weights.empty())
        vertex_joint_binding_buffer_size += sizeof(glm::vec4) * mesh_asset->vertex_bone_weights.size();

    rhi::BufferDesc stage_buffer_desc;
    stage_buffer_desc.size = vertex_position_buffer_size + vertex_varying_enable_blending_buffer_size +
        vertex_varying_buffer_size + vertex_joint_binding_buffer_size;
    stage_buffer_desc.usageBits = rhi::BUFFER_USAGE_TRANSFER_FROM_BIT;
    stage_buffer_desc.domain = rhi::BufferMemoryDomain::CPU;
    Ref<rhi::Buffer> stage_buffer = m_device->CreateBuffer(stage_buffer_desc, nullptr);
    void* stage_buffer_data = stage_buffer->GetMappedDataPtr();
    QK_CORE_VERIFY(stage_buffer_data, "Failed to map stage buffer");

    // upload vertex data to staging buffer
    uint64_t vertex_position_buffer_offset = 0;
    uint64_t vertex_varying_enable_blending_buffer_offset = vertex_position_buffer_offset + vertex_position_buffer_size;
    uint64_t vertex_varying_buffer_offset = vertex_varying_enable_blending_buffer_offset + vertex_varying_enable_blending_buffer_size;
    uint64_t vertex_joint_binding_buffer_offset = vertex_varying_buffer_offset + vertex_varying_buffer_size;

    glm::vec3* vertex_positions_buffer_data = reinterpret_cast<glm::vec3*>(reinterpret_cast<uintptr_t>(stage_buffer_data) + vertex_position_buffer_offset);
    uint8_t* vertex_varying_enable_blending_buffer_data = reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(stage_buffer_data) + vertex_varying_enable_blending_buffer_offset);
    uint8_t* vertex_varying_buffer_data = reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(stage_buffer_data) + vertex_varying_buffer_offset);
    uint8_t* vertex_joint_binding_buffer_data = reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(stage_buffer_data) + vertex_joint_binding_buffer_offset);

    size_t offset_in_vertex_varying_enable_blending_buffer = 0;
    size_t offset_in_vertex_varying_buffer = 0;
    size_t offset_in_vertex_joint_binding_buffer = 0;
    for (uint32_t i = 0; i < mesh_asset->GetVertexCount(); ++i)
    {
        vertex_positions_buffer_data[i] = mesh_asset->vertex_positions[i];

        if (!mesh_asset->vertex_normals.empty())
        {
            memcpy(vertex_varying_enable_blending_buffer_data + offset_in_vertex_varying_enable_blending_buffer,
                &mesh_asset->vertex_normals[i], sizeof(glm::vec3));
            offset_in_vertex_varying_enable_blending_buffer += sizeof(glm::vec3);
        }

        if (!mesh_asset->vertex_tangents.empty())
        {
            memcpy(vertex_varying_enable_blending_buffer_data + offset_in_vertex_varying_enable_blending_buffer,
                &mesh_asset->vertex_tangents[i], sizeof(glm::vec3));
            offset_in_vertex_varying_enable_blending_buffer += sizeof(glm::vec3);
        }

        if (!mesh_asset->vertex_uvs.empty())
        {
            memcpy(vertex_varying_buffer_data + offset_in_vertex_varying_buffer,
                &mesh_asset->vertex_uvs[i], sizeof(glm::vec2));
            offset_in_vertex_varying_buffer += sizeof(glm::vec2);
        }

        if (!mesh_asset->vertex_colors.empty())
        {
            memcpy(vertex_varying_buffer_data + offset_in_vertex_varying_buffer,
                &mesh_asset->vertex_colors[i], sizeof(glm::vec4));
            offset_in_vertex_varying_buffer += sizeof(glm::vec4);
        }

        if (!mesh_asset->vertex_bone_indices.empty())
        {
            memcpy(vertex_joint_binding_buffer_data + offset_in_vertex_joint_binding_buffer,
                &mesh_asset->vertex_bone_indices[i], sizeof(glm::uvec4));
            offset_in_vertex_joint_binding_buffer += sizeof(glm::uvec4);
        }

        if (!mesh_asset->vertex_bone_weights.empty())
        {
            memcpy(vertex_joint_binding_buffer_data + offset_in_vertex_joint_binding_buffer,
                &mesh_asset->vertex_bone_weights[i], sizeof(glm::vec4));
            offset_in_vertex_joint_binding_buffer += sizeof(glm::vec4);
        }

    }

    // copy staging buffer to gpu buffer
    rhi::BufferDesc buffer_desc;
    buffer_desc.domain = rhi::BufferMemoryDomain::GPU;
    buffer_desc.usageBits = rhi::BUFFER_USAGE_VERTEX_BUFFER_BIT | rhi::BUFFER_USAGE_TRANSFER_TO_BIT;

    buffer_desc.size = vertex_position_buffer_size;
    new_mesh_buffers->vbo_position = m_device->CreateBuffer(buffer_desc, nullptr);
    m_device->CopyBuffer(*new_mesh_buffers->vbo_position, *stage_buffer, vertex_position_buffer_size, 0, vertex_position_buffer_offset);

    if (vertex_varying_enable_blending_buffer_size > 0)
    {
        buffer_desc.size = vertex_varying_enable_blending_buffer_size;
        new_mesh_buffers->vbo_varying_enable_blending = m_device->CreateBuffer(buffer_desc, nullptr);
        m_device->CopyBuffer(*new_mesh_buffers->vbo_varying_enable_blending, *stage_buffer, vertex_varying_enable_blending_buffer_size, 0, vertex_varying_enable_blending_buffer_offset);
    }

    if (vertex_varying_buffer_size > 0)
    {
        buffer_desc.size = vertex_varying_buffer_size;
        new_mesh_buffers->vbo_varying = m_device->CreateBuffer(buffer_desc, nullptr);
        m_device->CopyBuffer(*new_mesh_buffers->vbo_varying, *stage_buffer, vertex_varying_buffer_size, 0, vertex_varying_buffer_offset);
    }

    if (vertex_joint_binding_buffer_size > 0)
    {
        buffer_desc.size = vertex_joint_binding_buffer_size;
        new_mesh_buffers->vbo_joint_binding = m_device->CreateBuffer(buffer_desc, nullptr);
        m_device->CopyBuffer(*new_mesh_buffers->vbo_joint_binding, *stage_buffer, vertex_joint_binding_buffer_size, 0, vertex_joint_binding_buffer_offset);
    }

    m_mesh_buffers[mesh_asset->GetAssetID()] = new_mesh_buffers;
    return new_mesh_buffers;
}

Ref<PBRMaterial> RenderResourceManager::RequestMateral(Ref<MaterialAsset> mat_asset)
{
    if (!mat_asset)
        return nullptr;

    auto find = m_materials.find(mat_asset->GetAssetID());
    if (find != m_materials.end())
        return find->second;

    Ref<PBRMaterial> new_material = CreateRef<PBRMaterial>();
    if (mat_asset->baseColorImage == 0)
    {
        new_material->textures[util::ecast(TextureKind::Albedo)] = image_white;
    }
    else
    {
        Ref<ImageAsset> image_asset = AssetManager::Get().GetAsset<ImageAsset>(mat_asset->baseColorImage);
        new_material->textures[util::ecast(TextureKind::Albedo)] = RequestImage(image_asset);
    }
    if (mat_asset->metallicRoughnessImage == 0)
    {
        new_material->textures[util::ecast(TextureKind::MetallicRoughness)] = image_white;
    }
    else
    {
        Ref<ImageAsset> image_asset = AssetManager::Get().GetAsset<ImageAsset>(mat_asset->metallicRoughnessImage);
        new_material->textures[util::ecast(TextureKind::MetallicRoughness)] = RequestImage(image_asset);
    }

    new_material->base_color = mat_asset->baseColorFactor;
    new_material->metallic_factor = mat_asset->metalicFactor;
    new_material->roughness_factor = mat_asset->roughNessFactor;
    new_material->draw_pipeline = mat_asset->alphaMode == AlphaMode::MODE_OPAQUE ? DrawPipeline::Opaque : DrawPipeline::AlphaBlend;
    new_material->hash = mat_asset->GetAssetID();

    if (mat_asset->vertexShaderPath == "" || mat_asset->fragmentShaderPath == "")
        new_material->shader_program = m_shader_library->program_staticMesh;
    else
        new_material->shader_program = m_shader_library->RequestGraphicsProgram(mat_asset->vertexShaderPath, mat_asset->fragmentShaderPath);

    m_materials[mat_asset->GetAssetID()] = new_material;

    return new_material;
}

Ref<rhi::PipeLine> RenderResourceManager::RequestGraphicsPSO(ShaderProgram& program, const rhi::RenderPassInfo2& rp, uint32_t mesh_attrib_mask, bool enableDepth, DrawPipeline draw_pipeline)
{

    auto& vertex_layout = RequestMeshVertexLayout(mesh_attrib_mask);
    ShaderVariantKey key;
    key.meshAttributeMask = mesh_attrib_mask;
    ShaderProgramVariant* programVariant = program.IsStatic()? program.GetPrecompiledVariant() : program.GetOrCreateVariant(key);
        
    rhi::PipelineColorBlendState& bs = draw_pipeline == DrawPipeline::Opaque ? blendState_opaque : blendState_transparent;
    rhi::PipelineDepthStencilState ds = depthStencilState_disabled;
    if (enableDepth)
        ds = (draw_pipeline == DrawPipeline::Opaque) ? depthStencilState_depthWrite : depthStencilState_depthTestOnly;

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
    for (const auto& attrib : vertex_layout.vertexAttribInfos)
    {
        h.u32(util::ecast(attrib.format));
        h.u32(attrib.offset);
        h.u32(attrib.binding);
    }

    for (const auto& b : vertex_layout.vertexBindInfos)
    {
        h.u32(b.binding);
        h.u32(b.stride);
        h.u32(util::ecast(b.inputRate));
    }

    util::Hash hash = h.get();
    auto it = m_cached_psos.find(hash);
    if (it != m_cached_psos.end())
    {
        return it->second;
    }
    else
    {
        rhi::GraphicPipeLineDesc desc = {};
        desc.vertShader = programVariant->GetShader(rhi::ShaderStage::STAGE_VERTEX);
        desc.fragShader = programVariant->GetShader(rhi::ShaderStage::STAGE_FRAGEMNT);
        desc.depthStencilState = ds;
        desc.blendState = bs;
        desc.rasterState = rasterizationState_fill;
        desc.topologyType = rhi::TopologyType::TRANGLE_LIST;
        desc.renderPassInfo = rp;
        if (vertex_layout.isValid())
            desc.vertexInputLayout = vertex_layout;

        Ref<rhi::PipeLine> newPipeline = m_device->CreateGraphicPipeLine(desc);
        m_cached_psos[hash] = newPipeline;

        return newPipeline;
    }
}

Ref<rhi::Image> RenderResourceManager::RequestImage(Ref<ImageAsset> image_asset)
{
    if (!image_asset)
		return nullptr;

    auto find = m_images.find(image_asset->GetAssetID());
    if (find != m_images.end())
        return find->second;

    rhi::ImageDesc desc = {};
    desc.width = image_asset->width;
    desc.height = image_asset->height;
    desc.depth = 1;
    desc.format = image_asset->format;
    desc.type = image_asset->type;
    desc.usageBits = rhi::IMAGE_USAGE_CAN_COPY_TO_BIT | rhi::IMAGE_USAGE_SAMPLING_BIT;
    desc.arraySize = image_asset->arraySize;
    desc.mipLevels = image_asset->mipLevels;
    desc.initialLayout = rhi::ImageLayout::SHADER_READ_ONLY_OPTIMAL;
    desc.generateMipMaps = false;

    Ref<rhi::Image> new_image = m_device->CreateImage(desc, image_asset->slices.data());
    m_images[image_asset->GetAssetID()] = new_image;

    return new_image;
}

rhi::VertexInputLayout& RenderResourceManager::RequestMeshVertexLayout(uint32_t meshAttributesMask)
{
    util::Hasher h;
    h.u32(meshAttributesMask);
    uint64_t hash = h.get();

    auto it = m_mesh_vertex_layouts.find(hash);
    if (it != m_mesh_vertex_layouts.end())
        return it->second;

    rhi::VertexInputLayout newLayout = {};

    // position
    if (meshAttributesMask & MESH_ATTRIBUTE_POSITION_BIT)
    {
        rhi::VertexInputLayout::VertexAttribInfo& attrib = newLayout.vertexAttribInfos.emplace_back();
        attrib.location = 0;
        attrib.binding = 0;
        attrib.format = rhi::VertexInputLayout::VertexAttribInfo::ATTRIB_FORMAT_VEC3;
        attrib.offset = 0;
    }

    // varying enable blending attris
    uint32_t enable_blending_buffer_offset = 0;
    if (meshAttributesMask & MESH_ATTRIBUTE_NORMAL_BIT)
    {
        rhi::VertexInputLayout::VertexAttribInfo& attrib = newLayout.vertexAttribInfos.emplace_back();
		attrib.location = 1;
		attrib.binding = 1;
		attrib.format = rhi::VertexInputLayout::VertexAttribInfo::ATTRIB_FORMAT_VEC3;
		attrib.offset = enable_blending_buffer_offset;
        enable_blending_buffer_offset += sizeof(glm::vec3);
    }
           
    if (meshAttributesMask & MESH_ATTRIBUTE_TANGENT_BIT)
    {
        rhi::VertexInputLayout::VertexAttribInfo& attrib = newLayout.vertexAttribInfos.emplace_back();
        attrib.location = 2;
        attrib.binding = 1;
        attrib.format = rhi::VertexInputLayout::VertexAttribInfo::ATTRIB_FORMAT_VEC3;
        attrib.offset = enable_blending_buffer_offset;
        enable_blending_buffer_offset += sizeof(glm::vec3);
    }

    // varying attris
    uint32_t varing_offset = 0;
    if (meshAttributesMask & MESH_ATTRIBUTE_UV_BIT)
    {
        rhi::VertexInputLayout::VertexAttribInfo& attrib = newLayout.vertexAttribInfos.emplace_back();
		attrib.location = 3;
		attrib.binding = 2;
		attrib.format = rhi::VertexInputLayout::VertexAttribInfo::ATTRIB_FORMAT_VEC2;
		attrib.offset = varing_offset;
        varing_offset += sizeof(glm::vec2);
    }

    if (meshAttributesMask & MESH_ATTRIBUTE_VERTEX_COLOR_BIT)
    {
        rhi::VertexInputLayout::VertexAttribInfo& attrib = newLayout.vertexAttribInfos.emplace_back();
		attrib.location = 4;
		attrib.binding = 2;
		attrib.format = rhi::VertexInputLayout::VertexAttribInfo::ATTRIB_FORMAT_VEC4;
		attrib.offset = varing_offset;
        varing_offset += sizeof(glm::vec4);
    }

    // joint binding attributes
    if (meshAttributesMask & MESH_ATTRIBUTE_BONE_INDEX_BIT)
    {
        rhi::VertexInputLayout::VertexAttribInfo& attrib = newLayout.vertexAttribInfos.emplace_back();
        attrib.location = 5;
        attrib.binding = 3;
        attrib.format = rhi::VertexInputLayout::VertexAttribInfo::ATTRIB_FORMAT_UVEC4;
        attrib.offset = 0;
    }
        
    if (meshAttributesMask & MESH_ATTRIBUTE_BONE_WEIGHT_BIT)
    {
        rhi::VertexInputLayout::VertexAttribInfo& attrib = newLayout.vertexAttribInfos.emplace_back();
        attrib.location = 6;
        attrib.binding = 3;
        attrib.format = rhi::VertexInputLayout::VertexAttribInfo::ATTRIB_FORMAT_VEC4;
        attrib.offset = sizeof(glm::uvec4);
    }

    // buffer binding infos
    rhi::VertexInputLayout::VertexBindInfo bindInfo = {};
    if (meshAttributesMask & MESH_ATTRIBUTE_POSITION_BIT)
    {
        bindInfo.binding = 0;
        bindInfo.stride = sizeof(glm::vec3);
        bindInfo.inputRate = rhi::VertexInputLayout::VertexBindInfo::INPUT_RATE_VERTEX;
        newLayout.vertexBindInfos.push_back(bindInfo);
    }

    if (enable_blending_buffer_offset > 0)
    {
        bindInfo.binding = 1;
		bindInfo.stride = enable_blending_buffer_offset;
		bindInfo.inputRate = rhi::VertexInputLayout::VertexBindInfo::INPUT_RATE_VERTEX;
		newLayout.vertexBindInfos.push_back(bindInfo);
    }

    if (varing_offset > 0)
    {
        bindInfo.binding = 2;
        bindInfo.stride = varing_offset;
        bindInfo.inputRate = rhi::VertexInputLayout::VertexBindInfo::INPUT_RATE_VERTEX;
        newLayout.vertexBindInfos.push_back(bindInfo);
    }

    if ((meshAttributesMask & MESH_ATTRIBUTE_BONE_INDEX_BIT) || (meshAttributesMask & MESH_ATTRIBUTE_BONE_WEIGHT_BIT))
    {
        bindInfo.binding = 3;
		bindInfo.stride = sizeof(glm::uvec4) + sizeof(glm::vec4);
		bindInfo.inputRate = rhi::VertexInputLayout::VertexBindInfo::INPUT_RATE_VERTEX;
		newLayout.vertexBindInfos.push_back(bindInfo);
    }

    m_mesh_vertex_layouts[hash] = newLayout;

    return m_mesh_vertex_layouts[hash];
}
}