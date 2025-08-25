#include "Quark/qkpch.h"
#include "Quark/Render/Utils/CommandListUtils.h"
#include "Quark/Render/RenderSystem.h"

namespace quark
{
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

		Ref<rhi::PipeLine> pso = manager.RequestFullScreenQuadPSO(*variant, depth_test, depth_write, depth_compare);




	}
}