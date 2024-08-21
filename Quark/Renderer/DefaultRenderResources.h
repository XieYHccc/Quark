#pragma once
#include <array>
#include "Quark/Core/Application.h"
#include "Quark/Graphic/Device.h"

namespace quark {
	class DefaultRenderResources {
	public:
		inline static Ref<graphic::Image> whiteImage;
		inline static Ref<graphic::Image> blackImage;
		inline static Ref<graphic::Image> checkboardImage;

		inline static Ref<graphic::Sampler> linearSampler;
		inline static Ref<graphic::Sampler> nearestSampler;
		inline static Ref<graphic::Sampler> cubeMapSampler;

		static void Init()
		{
			using namespace graphic;

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
		}

		static void ShutDown()
		{
			whiteImage.reset();
			blackImage.reset();
			checkboardImage.reset();
			linearSampler.reset();
			nearestSampler.reset();
			cubeMapSampler.reset();
		}
	};
}