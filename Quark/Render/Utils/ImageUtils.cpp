#include "Quark/qkpch.h"
#include "Quark/Render/Utils/ImageUtils.h"

namespace quark
{
Ref<rhi::Image> ConvertEquirectToCube(rhi::Device& device, const rhi::Image& equirect, float scale)
{
	using namespace rhi;
	uint32_t size = uint32_t(scale * std::max(equirect.GetDesc().width / 3, equirect.GetDesc().height / 2));

	ImageDesc desc = ImageDesc::RenderTarget(size, size, equirect.GetDesc().format);
	desc.mipLevels = 0;
	desc.arraySize = 6;
	desc.usageBits |= IMAGE_USAGE_SAMPLING_BIT | IMAGE_USAGE_CAN_COPY_FROM_BIT | IMAGE_USAGE_CAN_COPY_TO_BIT;
	desc.initialLayout = ImageLayout::COLOR_ATTACHMENT_OPTIMAL;

	Ref<Image> handle = device.CreateImage(desc, nullptr);
	CommandList* cmd = device.BeginCommandList();


	return nullptr;
}
}