#pragma once
#include "Quark/RHI/Device.h"
namespace quark
{

struct CommandListUtils
{
	static void DrawFullScreenQuad(rhi::CommandList& cmd, const std::string& vertex, const std::string& fragment,
		const std::vector<std::pair<std::string, int>>& defines = {});
	static void DrawFullScreenQuadDepth(rhi::CommandList& cmd, const std::string& vertex, const std::string& fragment,
		bool depth_test, bool depth_write, rhi::CompareOperation depth_compare,
		const std::vector<std::pair<std::string, int>>& defines = {});
	static void SetFullScreenQuadVertexState(rhi::CommandList& cmd);
	static void SetQuadVertexState(rhi::CommandList& cmd);

	static void SetupFullScreenQuad(rhi::CommandList& cmd, const std::string& vertex, const std::string& fragment,
		const std::vector<std::pair<std::string, int>>& defines = {},
		bool depth_test = false, bool depth_write = false,
		rhi::CompareOperation depth_compare = rhi::CompareOperation::ALWAYS);

	static void DrawFullScreenQuad(rhi::CommandList& cmd, unsigned instances = 1);
	static void DrawQuad(rhi::CommandList& cmd, unsigned instances = 1);

	static void ImageBarrier(rhi::CommandList& cmd, const rhi::Image& image,
		rhi::ImageLayout old_layout, rhi::ImageLayout new_layout,
		uint32_t src_stages, uint64_t src_access,
		uint32_t dst_stages, uint64_t dst_access);
};
} 
