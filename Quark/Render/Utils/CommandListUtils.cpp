#include "Quark/qkpch.h"
#include "Quark/Render/Utils/CommandListUtils.h"
#include "Quark/Render/RenderSystem.h"

namespace quark
{
	void CommandListUtils::DrawFullScreenQuad(rhi::CommandList& cmd, const std::string& vertex, const std::string& fragment, const std::vector<std::pair<std::string, int>>& defines)
	{
		DrawFullScreenQuadDepth(cmd, vertex, fragment, false, false, rhi::CompareOperation::ALWAYS, defines);
	}

	void CommandListUtils::DrawFullScreenQuadDepth(rhi::CommandList& cmd, const std::string& vertex, const std::string& fragment, bool depth_test, bool depth_write, rhi::CompareOperation depth_compare, const std::vector<std::pair<std::string, int>>& defines)
	{
		SetupFullScreenQuad(cmd, vertex, fragment, defines, depth_test, depth_write, depth_compare);
		DrawFullScreenQuad(cmd);

	}
	void CommandListUtils::SetFullScreenQuadVertexState(rhi::CommandList& cmd)
	{
		float* data = static_cast<float*>(cmd.AllocateVertexData(0, 6 * sizeof(float)));
		*data++ = -1.0f;
		*data++ = -3.0f;
		*data++ = -1.0f;
		*data++ = +1.0f;
		*data++ = +3.0f;
		*data++ = +1.0f;
	}	

	void CommandListUtils::SetupFullScreenQuad(rhi::CommandList& cmd, const std::string& vertex, const std::string& fragment,
		const std::vector<std::pair<std::string, int>>& defines,
		bool depth_test, bool depth_write,
		rhi::CompareOperation depth_compare)
	{
		auto& manager = RenderSystem::Get().GetRenderResourceManager();
		auto& shader_manager = RenderSystem::Get().GetRenderResourceManager().GetShaderLibrary();

		// program
		ShaderProgram* program = shader_manager.RequestGraphicsProgram(vertex, fragment);
		ShaderProgramVariant* variant = program->RequestVariant(defines);

		Ref<rhi::PipeLine> pso = manager.RequestFullScreenQuadPSO(*variant, cmd.GetCurrentRenderPassInfo(), depth_test, depth_write, depth_compare);
		cmd.BindPipeLine(*pso);
		SetFullScreenQuadVertexState(cmd);
	}

	void CommandListUtils::DrawFullScreenQuad(rhi::CommandList& cmd, unsigned instances)
	{
		cmd.Draw(3, instances, 0, 0);
	}

	void CommandListUtils::ImageBarrier(rhi::CommandList& cmd, const rhi::Image& image, rhi::ImageLayout old_layout, rhi::ImageLayout new_layout, uint32_t src_stages, uint64_t src_access, uint32_t dst_stages, uint64_t dst_access)
	{
		rhi::PipelineImageBarrier barrier;
		barrier.image = &image;
		barrier.layoutBefore = old_layout;
		barrier.layoutAfter = new_layout;
		barrier.srcStageBits = src_stages;
		barrier.srcMemoryAccessBits = src_access;
		barrier.dstStageBits = dst_stages;
		barrier.dstMemoryAccessBits = dst_access;
		cmd.PipeLineBarriers(nullptr, 0, &barrier, 1, nullptr, 0);
	}
}
