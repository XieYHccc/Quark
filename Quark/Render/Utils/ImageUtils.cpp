#include "Quark/qkpch.h"
#include "Quark/Render/Utils/ImageUtils.h"
#include "Quark/Render/RenderParameters.h"
#include "Quark/Render/RenderSystem.h"

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace quark
{

static void ComputeCubeFaceRenderTransform(const glm::vec3& center, unsigned face, glm::mat4& proj, glm::mat4& view, float znear, float zfar)
{
	static const glm::vec3 dirs[6] = {
		glm::vec3(1.0f, 0.0f, 0.0f),
		glm::vec3(-1.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, -1.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f),
		glm::vec3(0.0f, 0.0f, -1.0f),
	};

	static const glm::vec3 ups[6] = {
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, -1.0f),
		glm::vec3(0.0f, 0.0f, +1.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
	};

	view = glm::lookAt(center, center + dirs[face], ups[face]);
	proj = glm::scale(glm::vec3(-1.f, 1.f, 1.f)) * glm::perspective(glm::radians(45.f), 1.f, znear, zfar); // why -1?
}

Ref<rhi::Image> ConvertEquirectToCube(rhi::Device& device, const rhi::Image& equirect, float scale)
{
	using namespace rhi;
	uint32_t size = uint32_t(scale * std::max(equirect.GetDesc().width / 3, equirect.GetDesc().height / 2));

	ImageDesc desc = ImageDesc::RenderTarget(size, size, equirect.GetDesc().format);
	desc.type = ImageType::TYPE_CUBE;
	desc.mipLevels = 0;
	desc.arraySize = 6;
	desc.usageBits |= IMAGE_USAGE_SAMPLING_BIT | IMAGE_USAGE_CAN_COPY_FROM_BIT | IMAGE_USAGE_CAN_COPY_TO_BIT;
	desc.initialLayout = ImageLayout::COLOR_ATTACHMENT_OPTIMAL;

	Ref<Image> handle = device.CreateImage(desc, nullptr);
	CommandList* cmd = device.BeginCommandList();

	CameraParameters params;

	for (uint32_t i = 0; i < 6; i++)
	{
		ImageViewDesc view_desc = {};
		view_desc.layerCount = 1;
		view_desc.baseLayer = i;
		view_desc.format = desc.format;
		view_desc.levelCount = 1;
		view_desc.image = &equirect;
		Ref<ImageView> rt_view = device.CreateImageView(view_desc);

		RenderPassInfo rp = {};
		rp.colorAttachmentFormats[0] = view_desc.format;
		rp.numColorAttachments = 1;

		FrameBufferInfo fb = {};
		fb.colorAttachments[0] = rt_view.get();

		cmd->BeginRenderPass(rp, fb);
		glm::mat4 look, proj;
		ComputeCubeFaceRenderTransform(glm::vec3(0.f), i, proj, look, 0.1f, 100.f);

		params.inv_view_projection = glm::inverse(proj * look);
		memcpy(cmd->AllocateConstantData(0, 0, sizeof(params)), &params, sizeof(params));
		cmd->BindImageSampler(2, 0, equirect.GetDefaultView(), ImageLayout::SHADER_READ_ONLY_OPTIMAL, 
			*RenderSystem::Get().GetRenderResourceManager().sampler_linear);


	}


	return nullptr;
}
}