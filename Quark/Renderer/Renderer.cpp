#include "Quark/qkpch.h"
#include "Quark/Renderer/Renderer.h"
#include "Quark/Core/Application.h"

namespace quark {

using namespace graphic;


void Renderer::Init()
{
    graphic::Device* device = Application::Get().GetGraphicDevice();

    m_ShaderLibrary = CreateScope<ShaderLibrary>();

    // format_colorAttachment_main = Application::Get().GetGraphicDevice()->GetPresentImageFormat();

    ImageDesc desc = {};
    desc.depth = 1;
    desc.format = DataFormat::R8G8B8A8_UNORM;
    desc.initialLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL;
    desc.type = ImageType::TYPE_2D;
    desc.usageBits = IMAGE_USAGE_CAN_COPY_TO_BIT |IMAGE_USAGE_SAMPLING_BIT;
    desc.arraySize = 1;
    desc.mipLevels = 1;
    desc.generateMipMaps = false;

    constexpr uint32_t white = 0xFFFFFFFF;
    constexpr uint32_t black = 0x000000FF;
    constexpr uint32_t magenta = 0xFF00FFFF;

    // white image and black image
    {
        desc.width = 1;
        desc.height = 1;

        ImageInitData initData;
        initData.data = &white;
        initData.rowPitch = 4;
        initData.slicePitch = 1 * 1 * 4;

        image_white = device->CreateImage(desc, &initData);

        initData.data = &black;
        image_black = device->CreateImage(desc, &initData);

    }

    // Checkboard image
    {
        std::array<uint32_t, 32 * 32 > checkBoradpixelsData;
        for (int x = 0; x < 32; x++)
        {
            for (int y = 0; y < 32; y++)
                checkBoradpixelsData[y * 32 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
        }

        desc.width = 32;
        desc.height = 32;

        ImageInitData initData;
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
    
    // Depth stencil states
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

    // Rasterization state
	{
		rasterizationState_fill.cullMode = CullMode::NONE;
        rasterizationState_fill.frontFaceType = FrontFaceType::COUNTER_CLOCKWISE;
        rasterizationState_fill.polygonMode = PolygonMode::Fill;
        rasterizationState_fill.enableDepthClamp = false;
        rasterizationState_fill.enableAntialiasedLine = false;
        rasterizationState_fill.SampleCount = SampleCount::SAMPLES_1;
        rasterizationState_fill.lineWidth = 1.f;

        rasterizationState_wireframe.cullMode = CullMode::NONE;
        rasterizationState_wireframe.frontFaceType = FrontFaceType::COUNTER_CLOCKWISE;
        rasterizationState_wireframe.polygonMode = PolygonMode::Line;
        rasterizationState_wireframe.enableDepthClamp = false;
        rasterizationState_wireframe.enableAntialiasedLine = false;
        rasterizationState_wireframe.SampleCount = SampleCount::SAMPLES_1;
        rasterizationState_wireframe.lineWidth = 1.5f;
	}

    // Render Pass
    {
        // renderPassInfo_simpleMainPass.numColorAttachments = 1;
        // renderPassInfo_simpleMainPass.colorAttachmentFormats[0] = format_colorAttachment_main;
        // renderPassInfo_simpleMainPass.depthAttachmentFormat = format_depthAttachment_main;

        renderPassInfo2_simpleColorPass.numColorAttachments = 1;
        renderPassInfo2_simpleColorPass.colorAttachmentFormats[0] = format_colorAttachment_main;
        renderPassInfo2_simpleColorPass.sampleCount = SampleCount::SAMPLES_1;

        renderPassInfo2_simpleColorDepthPass.numColorAttachments = 1;
        renderPassInfo2_simpleColorDepthPass.colorAttachmentFormats[0] = format_colorAttachment_main;
        renderPassInfo2_simpleColorDepthPass.depthAttachmentFormat = format_depthAttachment_main;
        renderPassInfo2_simpleColorDepthPass.sampleCount = SampleCount::SAMPLES_1;

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

    // Pipelines
    {
        ShaderProgramVariant* precompiledSkyboxVariant = GetShaderLibrary().staticProgram_skybox->GetPrecompiledVariant();

        pipeline_skybox = precompiledSkyboxVariant->GetOrCreatePipeLine(depthStencilState_disabled,
            PipelineColorBlendState::create_disabled(1), rasterizationState_fill,
            renderPassInfo2_simpleColorDepthPass, vertexInputLayout_skybox);
    }

    QK_CORE_LOGI_TAG("Rernderer", "Renderer Initialized");
}

void Renderer::Shutdown()
{
    m_ShaderLibrary.reset();

    image_white.reset();
    image_black.reset();
    image_checkboard.reset();

    sampler_linear.reset();
    sampler_nearst.reset();
    sampler_cube.reset();
}

}