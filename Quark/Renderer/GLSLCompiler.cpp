#include "Quark/qkpch.h"
#include "Quark/Renderer/GLSLCompiler.h"
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <SPIRV/GlslangToSpv.h>
#include <SPIRV/GLSL.std.450.h>
#include <SPIRV/disassemble.h>
#include "Quark/Core/FileSystem.h"

namespace quark {

static glslang::EShTargetLanguage  s_TargetLanguage = glslang::EShTargetLanguage::EShTargetNone;

static EShLanguage FindShaderLanguage(graphic::ShaderStage stage)
{
	switch (stage)
	{
	case graphic::ShaderStage::STAGE_VERTEX:
		return EShLangVertex;
	case graphic::ShaderStage::STAGE_FRAGEMNT:
		return EShLangFragment;
	case graphic::ShaderStage::STAGE_COMPUTE:
		return EShLangCompute;
	default:
		CORE_LOGE("FindShaderLanguage: Unsupported shader stage");
		return EShLangCount;
	}
}
void GLSLCompiler::SetTarget(Target target)
{
	m_Target = target;
}

void GLSLCompiler::SetSource(std::string source, std::string sourcePath, graphic::ShaderStage stage)
{
	m_Source = std::move(source);
	m_SourcePath = std::move(sourcePath);
	m_ShaderStage = stage;
}

void GLSLCompiler::SetSourceFromFile(const std::string& filePath, graphic::ShaderStage stage)
{
	std::string source;
	if (!FileSystem::ReadFileText(filePath, source))
	{
		CORE_LOGE("GLSLCompiler::SetSourceFromFile: Failed to read file {}", filePath);
		return;
	}

	m_ShaderStage = stage;
	m_Source = std::move(source);
	m_SourcePath = filePath;
}

bool GLSLCompiler::Compile(std::string& outMessage, const std::vector<std::pair<std::string, int>>& defines, std::vector<uint32_t>& outSpirv)
{
	// Initialize glslang library.
	glslang::InitializeProcess();

	EShMessages messages = static_cast<EShMessages>(EShMsgDefault | EShMsgVulkanRules | EShMsgSpvRules);
	EShLanguage language = FindShaderLanguage(m_ShaderStage);

	std::string entryPoint = "main";
	const char* fileNameList[1] = { m_SourcePath.c_str()};
	const char* shaderSource = reinterpret_cast<const char*>(m_Source.data());

	glslang::TShader shader(language);
	shader.setStringsWithLengthsAndNames(&shaderSource, nullptr, fileNameList, 1);
	shader.setEntryPoint(entryPoint.c_str());
	shader.setSourceEntryPoint(entryPoint.c_str());

	return false;
}

};