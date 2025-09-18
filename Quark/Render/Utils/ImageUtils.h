#pragma once
#include "Quark/RHI/Device.h"
#include "Quark/RHI/TextureFormatLayout.h"
namespace quark
{
Ref<rhi::Image> ConvertEquirectToCube(rhi::Device& device, const rhi::Image& equirect, float scale);
Ref<rhi::Image> CreateIrradianceMap(rhi::Device& device, const rhi::Image& cube);
Ref<rhi::Image> CreatePrefilterMap(rhi::Device& device, const rhi::Image& cube);

struct ImageReadBack
{
	Ref<rhi::Buffer> buffer;
	rhi::ImageDesc image_desc;
	rhi::TextureFormatLayout layout;
};

ImageReadBack SaveImageToCpuBuffer(rhi::Device& device, const rhi::Image& image, rhi::QueueType queue_type);
bool SaveImageBufferToGtx(rhi::Device& device, ImageReadBack& readback, const char* path);

}