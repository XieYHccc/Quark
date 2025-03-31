#include <Quark/Quark.h>
#include <Quark/Asset/GLTFImporter.h>
#include <Quark/Asset/ImageImporter.h>
#include <Quark/Render/RenderContext.h>
#include <Quark/Render/RenderQueue.h>
#include <Quark/EntryPoint.h>

using namespace std;
using namespace quark;

class GLTFViewer : public Application
{
public:
	GLTFViewer(const ApplicationSpecification& specs)
		: Application(specs)
	{
		using namespace rhi;

		m_gltf_importer.Import("BuiltInResources/Gltf/FlightHelmet/glTF/FlightHelmet.gltf", GLTFImporter::ImportAll);

		// load cube map
		ImageImporter imageLoader;
		Ref<ImageAsset> cubeMap = imageLoader.ImportKtx2("BuiltInResources/Textures/Cubemaps/etc1s_cubemap_learnopengl.ktx2", true);
		m_cubeMapId = cubeMap->GetAssetID();
		AssetManager::Get().AddMemoryOnlyAsset(cubeMap);

		uint32_t width = m_window->GetFrambufferWidth();
		uint32_t height = m_window->GetFrambufferHeight();
		// Create depth image
		ImageDesc image_desc;
		image_desc.type = ImageType::TYPE_2D;
		auto ratio = Application::Get().GetWindow()->GetRatio();
		image_desc.width = width;
		image_desc.height = height;
		image_desc.depth = 1;
		image_desc.format = RenderSystem::Get().GetRenderResourceManager().format_depthAttachment_main;
		image_desc.arraySize = 1;
		image_desc.mipLevels = 1;
		image_desc.initialLayout = ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		image_desc.usageBits = IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		m_depth_attachment = RenderSystem::Get().GetDevice()->CreateImage(image_desc);
	}

	void OnUpdate(TimeStep ts) override final
	{

		auto scene = m_gltf_importer.GetScene();
		scene->OnUpdate(ts);

	}

	void OnRender(TimeStep ts) override final
	{
		auto& render_system = RenderSystem::Get();
		Ref<rhi::Device> rhi_device = render_system.GetDevice();
		Ref<Scene> scene = m_gltf_importer.GetScene();

		// update the rendering context.
		m_lighting_params.directional.color = glm::vec3(1.0f, 0.9f, 0.8f);
		m_lighting_params.directional.direction = glm::normalize(glm::vec3(1.f, 1.f, 1.f));
		m_render_context.SetLightingParameters(&m_lighting_params);
		glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 proj = glm::perspective(glm::radians(45.0f), 2500.f / 1600, 0.1f, 1000.0f);
		proj[1][1] *= -1;
		m_render_context.SetCamera(view, proj);

		// simple forward renderer, so we render opaque, transparent and background renderables in one go.
		m_visibility_list.clear();
		scene->GatherVisibleOpaqueRenderables(m_render_context.GetVisibilityFrustum(), m_visibility_list);
		m_render_queue.Reset();
		m_render_queue.PushRenderables(m_render_context, m_visibility_list.data(), m_visibility_list.size());
		m_render_queue.Sort();

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
			fb_info.colorAttachments[0] = swap_chain_image;
			fb_info.clearColors[0] = { 0.2f, 0.2f, 0.2f, 1.f };
			fb_info.depthAttachment = m_depth_attachment.get();
			fb_info.depthAttachmentLoadOp = rhi::FrameBufferInfo::AttachmentLoadOp::CLEAR;
			fb_info.depthAttachmentStoreOp = rhi::FrameBufferInfo::AttachmentStoreOp::STORE;
			fb_info.clearDepthStencil.depth_stencil = { 1.f, 0 };

			rhi::RenderPassInfo2 render_pass_info = render_system.GetRenderResourceManager().renderPassInfo_swapchainPass;
			render_pass_info.depthAttachmentFormat = m_depth_attachment->GetDesc().format;

			cmd->BeginRenderPass(render_pass_info, fb_info);
			cmd->SetViewPort(viewport);
			cmd->SetScissor(scissor);
			render_system.Flush(*cmd, m_render_queue, m_render_context);
			cmd->EndRenderPass();

			// ui pass
			fb_info.colorAttatchemtsLoadOp[0] = rhi::FrameBufferInfo::AttachmentLoadOp::LOAD;
			fb_info.colorAttatchemtsStoreOp[0] = rhi::FrameBufferInfo::AttachmentStoreOp::STORE;
			cmd->BeginRenderPass(render_system.GetRenderResourceManager().renderPassInfo_swapchainPass, fb_info);
			UI::Get()->OnRender(cmd);
			cmd->EndRenderPass();

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

	GLTFImporter m_gltf_importer;
	RenderContext m_render_context;
	RenderQueue m_render_queue;
	LightingParameters m_lighting_params;
	VisibilityList m_visibility_list;
	Ref<rhi::Image> m_depth_attachment;
	AssetID m_cubeMapId;
};

namespace quark
{
Application* CreateApplication(int argc, char** argv)
{
	ApplicationSpecification specs;
	specs.uiSpecs.flags = 0;
	specs.title = "GLTFViewer";
	specs.width = 2500;
	specs.height = 1600;
	specs.isFullScreen = false;
	specs.workingDirectory = "D:/Dev/Quark/bin";

	return new GLTFViewer(specs);
}
}