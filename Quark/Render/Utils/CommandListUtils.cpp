#include "Quark/qkpch.h"
#include "Quark/Render/Utils/CommandListUtils.h"
#include "Quark/Render/RenderSystem.h"

namespace quark
{
	void CommandListUtil::DrawFullScreenQuad(rhi::CommandList& cmd, const std::string& vertex, const std::string& fragment, const std::vector<std::pair<std::string, int>>& defines)
	{
		DrawFullScreenQuadDepth(cmd, vertex, fragment, false, false, rhi::CompareOperation::ALWAYS, defines);
	}

	void CommandListUtil::DrawFullScreenQuadDepth(rhi::CommandList& cmd, const std::string& vertex, const std::string& fragment, bool depth_test, bool depth_write, rhi::CompareOperation depth_compare, const std::vector<std::pair<std::string, int>>& defines)
	{
		SetupFullScreenQuad(cmd, vertex, fragment, defines, depth_test, depth_write, depth_compare);
		DrawFullScreenQuad(cmd);

	}
	void CommandListUtil::SetFullScreenQuadVertexState(rhi::CommandList& cmd)
	{
		float* data = static_cast<float*>(cmd.AllocateVertexData(0, 6 * sizeof(float)));
		*data++ = -1.0f;
		*data++ = -3.0f;
		*data++ = -1.0f;
		*data++ = +1.0f;
		*data++ = +3.0f;
		*data++ = +1.0f;
	}	

	void CommandListUtil::SetupFullScreenQuad(rhi::CommandList& cmd, const std::string& vertex, const std::string& fragment,
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

	void CommandListUtil::DrawFullScreenQuad(rhi::CommandList& cmd, unsigned instances)
	{
		cmd.Draw(3, instances, 0, 0);
	}
}