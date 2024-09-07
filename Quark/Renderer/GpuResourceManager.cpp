#include "Quark/qkpch.h"
#include "Quark/Renderer/GpuResourceManager.h"
#include "Quark/Core/Application.h"

namespace quark {

using namespace graphic;


void GpuResourceManager::Init()
{

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

        whiteImage = Application::Get().GetGraphicDevice()->CreateImage(desc, &initData);

        initData.data = &black;
        blackImage = Application::Get().GetGraphicDevice()->CreateImage(desc, &initData);

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

        checkboardImage = Application::Get().GetGraphicDevice()->CreateImage(desc, &initData);
    }


    // Samplers
    {
        SamplerDesc desc = {};
        desc.minFilter = SamplerFilter::LINEAR;
        desc.magFliter = SamplerFilter::LINEAR;
        desc.addressModeU = SamplerAddressMode::REPEAT;
        desc.addressModeV = SamplerAddressMode::REPEAT;
        desc.addressModeW = SamplerAddressMode::REPEAT;

        linearSampler = Application::Get().GetGraphicDevice()->CreateSampler(desc);

        desc.minFilter = SamplerFilter::NEAREST;
        desc.magFliter = SamplerFilter::NEAREST;

        nearestSampler = Application::Get().GetGraphicDevice()->CreateSampler(desc);

        // Cubemap sampler
        desc.minFilter = SamplerFilter::LINEAR;
        desc.magFliter = SamplerFilter::LINEAR;
        desc.addressModeU = SamplerAddressMode::CLAMPED_TO_EDGE;
        desc.addressModeV = SamplerAddressMode::CLAMPED_TO_EDGE;
        desc.addressModeW = SamplerAddressMode::CLAMPED_TO_EDGE;

        cubeMapSampler = Application::Get().GetGraphicDevice()->CreateSampler(desc);

    }
    
    // Depth stencil states
	{
        depthDisabledState.enableDepthTest = false;
        depthDisabledState.enableDepthWrite = false;

        depthTestState.enableDepthTest = true;
        depthTestState.enableDepthWrite = false;
        depthTestState.depthCompareOp = CompareOperation::LESS_OR_EQUAL;
        depthTestWriteState.enableStencil = false;

        depthTestWriteState.enableDepthTest = true;
        depthTestWriteState.enableDepthWrite = true;
        depthTestWriteState.depthCompareOp = CompareOperation::LESS_OR_EQUAL;
        depthTestWriteState.enableStencil = false;
	}

    // Rasterization state
	{
		defaultFillRasterizationState.cullMode = CullMode::NONE;
		defaultFillRasterizationState.frontFaceType = FrontFaceType::COUNTER_CLOCKWISE;
        defaultFillRasterizationState.polygonMode = PolygonMode::Fill;
		defaultFillRasterizationState.enableDepthClamp = false;
		defaultFillRasterizationState.enableAntialiasedLine = false;
		defaultFillRasterizationState.SampleCount = SampleCount::SAMPLES_1;
		defaultFillRasterizationState.lineWidth = 1.f;

        wireframeRasterizationState.cullMode = CullMode::NONE;
        wireframeRasterizationState.frontFaceType = FrontFaceType::COUNTER_CLOCKWISE;
        wireframeRasterizationState.polygonMode = PolygonMode::Line;
        wireframeRasterizationState.enableDepthClamp = false;
        wireframeRasterizationState.enableAntialiasedLine = false;
        wireframeRasterizationState.SampleCount = SampleCount::SAMPLES_1;
        wireframeRasterizationState.lineWidth = 1.5f;
	}

    // Render Pass
    {
        defaultOneColorWithDepthRenderPassInfo.numColorAttachments = 1;
        defaultOneColorWithDepthRenderPassInfo.colorAttachmentFormats[0] = DataFormat::B8G8R8A8_UNORM;
        defaultOneColorWithDepthRenderPassInfo.depthAttachmentFormat = DataFormat::D32_SFLOAT;

    }

    CORE_LOGI("[GpuResourceManager]: Initialized");
}

void GpuResourceManager::Shutdown()
{
    whiteImage.reset();
    blackImage.reset();
    checkboardImage.reset();

    linearSampler.reset();
    nearestSampler.reset();
    cubeMapSampler.reset();
}


}