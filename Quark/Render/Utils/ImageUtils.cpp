#include "Quark/qkpch.h"
#include "Quark/Render/Utils/ImageUtils.h"
#include "Quark/Render/Utils/CommandListUtils.h"
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
		glm::vec3(0.0f, -1.0f, 0.0f),
		glm::vec3(0.0f, +1.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f),
		glm::vec3(0.0f, 0.0f, -1.0f),
	};

	static const glm::vec3 ups[6] = {
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, +1.0f),
		glm::vec3(0.0f, 0.0f, -1.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
	};

	view = glm::lookAt(center, center + dirs[face], ups[face]);
	proj = glm::scale(glm::vec3(-1.f, 1.f, 1.f)) * glm::perspective(glm::radians(90.f), 1.f, znear, zfar);
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
		view_desc.image = handle.get();
		Ref<ImageView> rt_view = device.CreateImageView(view_desc);

		RenderPassInfo rp = {};
		rp.colorAttachmentFormats[0] = view_desc.format;
		rp.numColorAttachments = 1;

		FrameBufferInfo fb = {};
		fb.colorAttachments[0] = rt_view.get();

		// Viewport and scissor
		rhi::Viewport viewport;
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = (float)desc.width;
		viewport.height = (float)desc.height;
		viewport.minDepth = 0;
		viewport.maxDepth = 1;

		rhi::Scissor scissor;
		scissor.extent.width = (int)viewport.width;
		scissor.extent.height = (int)viewport.height;
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		cmd->SetViewPort(viewport);
		cmd->SetScissor(scissor);

		cmd->BeginRenderPass(rp, fb);
		
		glm::mat4 look, proj;
		ComputeCubeFaceRenderTransform(glm::vec3(0.f), i, proj, look, 0.1f, 100.f);

		params.inv_local_view_projection = glm::inverse(proj * look);
		memcpy(cmd->AllocateConstantData(0, 0, sizeof(params)), &params, sizeof(params));
		cmd->BindImageSampler(2, 0, equirect.GetDefaultView(), ImageLayout::SHADER_READ_ONLY_OPTIMAL, 
			*RenderSystem::Get().GetRenderResourceManager().sampler_linear);

		CommandListUtils::DrawFullScreenQuad(*cmd, "BuiltInResources/Shaders/skybox_quad.vert", "BuiltInResources/Shaders/equirect_to_cube.frag");

		cmd->EndRenderPass();

	}

	cmd->GenerateMipmap(*handle, ImageLayout::COLOR_ATTACHMENT_OPTIMAL);
	CommandListUtils::ImageBarrier(*cmd, *handle, ImageLayout::TRANSFER_SRC_OPTIMAL, ImageLayout::SHADER_READ_ONLY_OPTIMAL,
		PIPELINE_STAGE_TRANSFER_BIT, 0, PIPELINE_STAGE_FRAGMENT_SHADER_BIT | PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		BARRIER_ACCESS_SHADER_SAMPLED_READ_BIT | BARRIER_ACCESS_SHADER_STORAGE_READ_BIT);
	device.SubmitCommandList(cmd);

	return handle;
}
Ref<rhi::Image> CreateIrradianceMap(rhi::Device& device, const rhi::Image& cube)
{
	using namespace rhi;
	uint32_t size = 32;
	rhi::ImageDesc desc = ImageDesc::RenderTarget(size, size, DataFormat::R16G16B16A16_SFLOAT);
	desc.mipLevels = 1;
	desc.arraySize = 6;
	desc.type = ImageType::TYPE_CUBE;
	desc.usageBits |= IMAGE_USAGE_SAMPLING_BIT | IMAGE_USAGE_CAN_COPY_FROM_BIT;
	desc.initialLayout = ImageLayout::COLOR_ATTACHMENT_OPTIMAL;

	Ref<Image> handle = device.CreateImage(desc);
	CommandList* cmd = device.BeginCommandList();

	CameraParameters params;
	for (uint32_t i = 0; i < 6; i++)
	{
		ImageViewDesc view_desc;
		view_desc.layerCount = 1;
		view_desc.baseLayer = i;
		view_desc.format = desc.format;
		view_desc.baseLevel = 0;
		view_desc.levelCount = 1;

		Ref<ImageView> rt_view = device.CreateImageView(view_desc);
		RenderPassInfo rp_info;
		rp_info.numColorAttachments = 1;
		rp_info.colorAttachmentFormats[0] = desc.format;
		FrameBufferInfo fb_info;
		fb_info.colorAttachments[0] = rt_view.get();
		fb_info.colorAttatchemtsLoadOp[0] = FrameBufferInfo::AttachmentLoadOp::DONTCARE;
		
		cmd->BeginRenderPass(rp_info, fb_info);

		glm::mat4 look, proj;
		ComputeCubeFaceRenderTransform(glm::vec3(0.f), i, proj, look, 0.1f, 100.f);
		params.inv_local_view_projection = glm::inverse(proj * look);
		memcpy(cmd->AllocateConstantData(0, 0, sizeof(params)), &params, sizeof(params));
		cmd->BindImageSampler(2, 0, cube.GetDefaultView(), ImageLayout::SHADER_READ_ONLY_OPTIMAL,
			*RenderSystem::Get().GetRenderResourceManager().sampler_linear);

	}
	return Ref<Image>();
}
}
