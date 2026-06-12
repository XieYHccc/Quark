#include <Quark/Quark.h>
#include <Quark/Asset/GLTFImporter.h>
#include <Quark/Asset/ImageImporter.h>
#include <Quark/Render/RenderContext.h>
#include <Quark/Render/Utils/CommandListUtils.h>
#include <Quark/EntryPoint.h>

using namespace std;
using namespace quark;

class TextureViewer : public Application
{
public:
	TextureViewer(const ApplicationSpecification& specs)
		: Application(specs)
	{
		using namespace rhi;

		// load texture
		ImageImporter imageLoader;
		m_texture = imageLoader.ImportHdr("BuiltInResources/Textures/Hdr/newport_loft.hdr");
		// m_texture = imageLoader.ImportStb("BuiltInResources/Textures/grid_box_grey.png");

		auto& resource_manager= RenderSystem::Get().GetRenderResourceManager();
		m_texture_gpu_resouce = resource_manager.RequestImage(m_texture);
	}

	void OnUpdate(TimeStep ts) override final
	{

	}

	void OnRender(TimeStep ts) override final
	{
		auto& render_system = RenderSystem::Get();
		Ref<rhi::Device> rhi_device = render_system.GetDevice();

		// Rendering commands
		if (rhi_device->BeiginFrame(ts))
		{
			auto* cmd = rhi_device->BeginCommandList();
			auto* swap_chain_image = rhi_device->GetPresentImage();

			// Viewport and scissor
			rhi::Viewport viewport;
			viewport.x = 0;
			viewport.y = 0;
			viewport.width = (float)swap_chain_image->GetDesc().width;
			viewport.height = (float)swap_chain_image->GetDesc().height;
			viewport.minDepth = 0;
			viewport.maxDepth = 1;

			rhi::Scissor scissor;
			scissor.extent.width = (int)viewport.width;
			scissor.extent.height = (int)viewport.height;
			scissor.offset.x = 0;
			scissor.offset.y = 0;
			cmd->SetViewPort(viewport);
			cmd->SetScissor(scissor);

			// main pass
			rhi::PipelineImageBarrier image_barrier;
			image_barrier.image = swap_chain_image;
			image_barrier.srcStageBits = rhi::PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			image_barrier.srcMemoryAccessBits = 0;
			image_barrier.dstStageBits = rhi::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			image_barrier.dstMemoryAccessBits = rhi::BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			image_barrier.layoutBefore = rhi::ImageLayout::UNDEFINED;
			image_barrier.layoutAfter = rhi::ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
			cmd->PipeLineBarriers(nullptr, 0, &image_barrier, 1, nullptr, 0);

			rhi::FrameBufferInfo fb_info;
			fb_info.colorAttatchemtsLoadOp[0] = rhi::FrameBufferInfo::AttachmentLoadOp::CLEAR;
			fb_info.colorAttatchemtsStoreOp[0] = rhi::FrameBufferInfo::AttachmentStoreOp::STORE;
			fb_info.colorAttachments[0] = &swap_chain_image->GetDefaultView();
			fb_info.clearColors[0] = { 0.2f, 0.2f, 0.3f, 1.f };

			rhi::RenderPassInfo render_pass_info = render_system.GetRenderResourceManager().renderPassInfo_swapchainPass;

			cmd->BeginRegion("Main pass");
			cmd->BeginRenderPass(render_pass_info, fb_info);
			cmd->BindImageSampler(0, 0, m_texture_gpu_resouce->GetDefaultView(), rhi::ImageLayout::SHADER_READ_ONLY_OPTIMAL, *render_system.GetRenderResourceManager().sampler_nearst);
			CommandListUtil::DrawFullScreenQuad(*cmd, "BuiltInResources/Shaders/quad.vert","BuiltInResources/Shaders/blit.frag");
			cmd->EndRenderPass();
			cmd->EndRegion();

		//	// ui pass
		//	rhi::PipelineMemoryBarrier mem_barrier;
		//	mem_barrier.srcStageBits = rhi::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		//	mem_barrier.srcMemoryAccessBits = rhi::BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		//	mem_barrier.dstStageBits = rhi::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		//	mem_barrier.dstMemoryAccessBits = rhi::BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		//	cmd->PipeLineBarriers(&mem_barrier, 1, nullptr, 0, nullptr, 0);

		//	fb_info.colorAttatchemtsLoadOp[0] = rhi::FrameBufferInfo::AttachmentLoadOp::LOAD;
		//	fb_info.colorAttatchemtsStoreOp[0] = rhi::FrameBufferInfo::AttachmentStoreOp::STORE;
		//	cmd->BeginRenderPass(render_system.GetRenderResourceManager().renderPassInfo_swapchainPass, fb_info);
		//	UI::Get()->OnRender(cmd);
		//	cmd->EndRenderPass();
		//	
			// transit swapchain image to present layout for presenting
			rhi::PipelineImageBarrier present_barrier;
			present_barrier.image = swap_chain_image;
			present_barrier.srcStageBits = rhi::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			present_barrier.srcMemoryAccessBits = rhi::BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			present_barrier.dstStageBits = rhi::PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			present_barrier.dstMemoryAccessBits = 0;
			present_barrier.layoutBefore = rhi::ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
			present_barrier.layoutAfter = rhi::ImageLayout::PRESENT;
			cmd->PipeLineBarriers(nullptr, 0, &present_barrier, 1, nullptr, 0);

			// Submit graphic command list
			rhi_device->SubmitCommandList(cmd);
			rhi_device->EndFrame(ts);
		}
	}

	void OnImGuiUpdate() override final
	{
		UI::Get()->BeginFrame();

		if (ImGui::Begin("Debug"))
		{
			ImGui::Text("FPS: %f", m_status.fps);
			ImGui::Text("Frame Time: %f ms", m_status.lastFrameDuration);

		}
		ImGui::End();

		UI::Get()->EndFrame();
	}

	Ref<ImageAsset> m_texture;
	Ref<rhi::Image> m_texture_gpu_resouce;
	Ref<Skybox> m_skybox;
};

namespace quark
{
	Application* CreateApplication(int argc, char** argv)
	{
		ApplicationSpecification specs;
		specs.uiSpecs.flags = 0;
		specs.title = "TextureViewer";
		specs.width = 1440;
		specs.height = 960;
		specs.isFullScreen = false;

		return new TextureViewer(specs);
	}
}