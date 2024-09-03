#pragma once
#include "Quark/Graphic/Shader.h"

namespace quark {


enum class Target
{
	VULKAN_VERSION_1_3,
	VULKAN_VERSION_1_1,
	
};

/// Helper class to generate SPIRV code from GLSL source
/// Currently only support compiling for one shader stage and vulkan 1.3
class GLSLCompiler {
public:
	GLSLCompiler() = default;

	void SetTarget(Target target);

	void SetSource(std::string source, std::string sourcePath, graphic::ShaderStage stage);
	void SetSourceFromFile(const std::string& filePath, graphic::ShaderStage stage);

	bool Compile(std::string& outMessage, const std::vector<std::pair<std::string, int>>& defines, std::vector<uint32_t>& outSpirv);

private:
	std::string m_SourcePath;
	std::string m_Source;

	Target m_Target = Target::VULKAN_VERSION_1_1;

	graphic::ShaderStage m_ShaderStage = graphic::ShaderStage::MAX_ENUM;


};
}