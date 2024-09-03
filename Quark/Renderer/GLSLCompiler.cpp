#include "Quark/qkpch.h"
#include "Quark/Renderer/GLSLCompiler.h"
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <StandAlone/DirStackFileIncluder.h>
#include <SPIRV/GlslangToSpv.h>
#include <SPIRV/GLSL.std.450.h>
#include <SPIRV/disassemble.h>
#include "Quark/Core/FileSystem.h"
#include "Quark/Core/Util/Hash.h"

namespace quark {

static glslang::EShTargetLanguage  s_TargetLanguage = glslang::EShTargetLanguage::EShTargetSpv;

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

void CompileOptions::AddDefinitions(const std::vector<std::string>& definitions)
{
	for (auto& def : definitions)
	{
		AddDefine(def);
	}
}

void CompileOptions::AddDefine(const std::string& def)
{
	m_Processes.push_back("define-macro ");
	m_Processes.back().append(def);

	std::string tmp_def = def;
	FixLine(tmp_def);

	// The "=" needs to turn into a space
	size_t pos_equal = tmp_def.find_first_of("=");
	if (pos_equal != std::string::npos)
	{
		tmp_def[pos_equal] = ' ';
	}

	m_Preamble.append("#define " + tmp_def + "\n");

}

void CompileOptions::AddUndefine(const std::string& undef)
{
	std::string tmpUndef = undef;
	FixLine(tmpUndef);

	m_Processes.push_back("undef-macro ");
	m_Processes.back().append(undef);

	m_Preamble.append("#undef " + tmpUndef + "\n");

}


void CompileOptions::FixLine(std::string& line)
{
	// Can't go past a newline in the line
	const size_t end = line.find_first_of("\n");
	if (end != line.npos)
		line = line.substr(0, end);
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

bool GLSLCompiler::Compile(std::string& outMessages, std::vector<uint32_t>& outSpirv, const CompileOptions& ops)
{
	// Initialize glslang library.
	glslang::InitializeProcess();

	if (m_Source.empty())
	{
		CORE_LOGE("GLSLCompiler::Compile: Source is empty. Please set source first");
		return false;
	}

	EShMessages messages = static_cast<EShMessages>(EShMsgDefault | EShMsgVulkanRules | EShMsgSpvRules);
	EShLanguage language = FindShaderLanguage(m_ShaderStage);

	std::string entryPoint = "main";
	const char* fileNameList[1] = { m_SourcePath.c_str()};
	const char* shaderSource = reinterpret_cast<const char*>(m_Source.data());

	glslang::TShader shader(language);
	shader.setStringsWithLengthsAndNames(&shaderSource, nullptr, fileNameList, 1);
	shader.setEntryPoint(entryPoint.c_str());
	shader.setSourceEntryPoint(entryPoint.c_str());
	shader.setPreamble(ops.GetPreamble().c_str());
	shader.addProcesses(ops.GetProcesses());

	// TODO: Remove hard coded Env Client and Target
	shader.setEnvTarget(s_TargetLanguage, m_Target == Target::VULKAN_VERSION_1_3? glslang::EShTargetSpv_1_6 : glslang::EShTargetSpv_1_3);
	shader.setEnvClient(glslang::EShClientVulkan, m_Target == Target::VULKAN_VERSION_1_3 ? glslang::EShTargetVulkan_1_3 : glslang::EShTargetVulkan_1_1);


	DirStackFileIncluder includeDir;
	includeDir.pushExternalLocalDirectory("BuiltInReources/Shaders");

	if (!shader.parse(GetDefaultResources(), 100, false, messages, includeDir))
	{
		outMessages = std::string(shader.getInfoLog()) + "\n" + std::string(shader.getInfoDebugLog());
		return false;
	}

	// Add shader to new program object.
	glslang::TProgram program;
	program.addShader(&shader);

	// Link program.
	if (!program.link(messages))
	{
		outMessages = std::string(program.getInfoLog()) + "\n" + std::string(program.getInfoDebugLog());
		return false;
	}

	// Save any info log that was generated.
	if (shader.getInfoLog())
	{
		outMessages += std::string(shader.getInfoLog()) + "\n" + std::string(shader.getInfoDebugLog()) + "\n";
	}

	if (program.getInfoLog())
	{
		outMessages += std::string(program.getInfoLog()) + "\n" + std::string(program.getInfoDebugLog());
	}

	glslang::TIntermediate* intermediate = program.getIntermediate(language);

	// Translate to SPIRV.
	if (!intermediate)
	{
		outMessages += "Failed to get shared intermediate code.\n";
		return false;
	}

	spv::SpvBuildLogger logger;

	outSpirv.clear();
	glslang::GlslangToSpv(*intermediate, outSpirv, &logger);

	outMessages += logger.getAllMessages() + "\n";

	// Shutdown glslang library.
	glslang::FinalizeProcess();

	return true;
}


};