#include "Quark/qkpch.h"
#include "Quark/Render/GLSLCompiler.h"
#include "Quark/Core/FileSystem.h"
#include "Quark/Core/Util/Hash.h"
#include "Quark/Core/Util/StringUtils.h"

#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <StandAlone/DirStackFileIncluder.h>
#include <SPIRV/GlslangToSpv.h>
#include <SPIRV/GLSL.std.450.h>
#include <SPIRV/disassemble.h>

namespace quark {

static glslang::EShTargetLanguage  s_TargetLanguage = glslang::EShTargetLanguage::EShTargetSpv;

static EShLanguage FindShaderLanguage(rhi::ShaderStage stage)
{
	switch (stage)
	{
	case rhi::ShaderStage::STAGE_VERTEX:
		return EShLangVertex;
	case rhi::ShaderStage::STAGE_FRAGEMNT:
		return EShLangFragment;
	case rhi::ShaderStage::STAGE_COMPUTE:
		return EShLangCompute;
	default:
		QK_CORE_LOGE_TAG("Rernderer", "FindShaderLanguage: Unsupported shader stage");
		return EShLangCount;
	}
}

void GLSLCompiler::CompileOptions::AddDefinitions(const std::vector<std::string>& definitions)
{
	for (auto& def : definitions)
	{
		AddDefine(def);
	}
}

void GLSLCompiler::CompileOptions::AddDefine(const std::string& def)
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

void GLSLCompiler::CompileOptions::AddUndefine(const std::string& undef)
{
	std::string tmpUndef = undef;
	FixLine(tmpUndef);

	m_Processes.push_back("undef-macro ");
	m_Processes.back().append(undef);

	m_Preamble.append("#undef " + tmpUndef + "\n");

}


void GLSLCompiler::CompileOptions::FixLine(std::string& line)
{
	// Can't go past a newline in the line
	const size_t end = line.find_first_of("\n");
	if (end != line.npos)
		line = line.substr(0, end);
}

void GLSLCompiler::SetTarget(Target target)
{
	m_target = target;
}

void GLSLCompiler::SetSource(std::string source, std::string sourcePath, rhi::ShaderStage stage)
{
	Clear();

	m_source = std::move(source);
	m_sourcePath = std::move(sourcePath);
	m_shaderStage = stage;
}

void GLSLCompiler::SetSourceFromFile(const std::string& filePath, rhi::ShaderStage stage)
{
	Clear();

	std::string source;
	if (!FileSystem::ReadFileText(filePath, source))
	{
		QK_CORE_LOGE_TAG("Rernderer", "GLSLCompiler::SetSourceFromFile: Failed to read file {}", filePath);
		return;
	}

	m_shaderStage = stage;
	m_source = std::move(source);
	m_sourcePath = filePath;
}

bool GLSLCompiler::Compile(std::string& outMessages, std::vector<uint32_t>& outSpirv, const CompileOptions& ops)
{

	// Initialize glslang library.
	glslang::InitializeProcess();

	if (m_source.empty())
	{
		QK_CORE_LOGE_TAG("Rernderer", "GLSLCompiler::Compile: Source is empty. Please set source first");
		return false;
	}

	if (!m_isPreprocessed)
	{
		PreProcess();
		m_isPreprocessed = true;
	}

	EShMessages messages = static_cast<EShMessages>(EShMsgDefault | EShMsgVulkanRules | EShMsgSpvRules);
	EShLanguage language = FindShaderLanguage(m_shaderStage);

	std::string entryPoint = "main";
	const char* fileNameList[1] = { m_sourcePath.c_str()};
	const char* shaderSource = reinterpret_cast<const char*>(m_preprocessedSource.data());

	glslang::TShader shader(language);
	shader.setStringsWithLengthsAndNames(&shaderSource, nullptr, fileNameList, 1);
	shader.setEntryPoint(entryPoint.c_str());
	shader.setSourceEntryPoint(entryPoint.c_str());
	shader.setPreamble(ops.GetPreamble().c_str());
	shader.addProcesses(ops.GetProcesses());

	// TODO: Remove hard coded Env Client and Target
	shader.setEnvTarget(s_TargetLanguage, m_target == Target::VULKAN_VERSION_1_3? glslang::EShTargetSpv_1_6 : glslang::EShTargetSpv_1_3);
	shader.setEnvClient(glslang::EShClientVulkan, m_target == Target::VULKAN_VERSION_1_3 ? glslang::EShTargetVulkan_1_3 : glslang::EShTargetVulkan_1_1);


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

void GLSLCompiler::Clear()
{
	m_source.clear();
	m_sourcePath.clear();
	m_preprocessedSource.clear();
	m_includeDependencies.clear();
	m_isPreprocessed = false;

}

void GLSLCompiler::PreProcess()
{
	m_preprocessedSource.clear();

	ParseSource(m_source, m_sourcePath, m_preprocessedSource);
}

bool GLSLCompiler::ParseSource(const std::string& source, const std::string sourcePath, std::string& outParsedResult)
{
	outParsedResult = std::string{};

	std::vector<std::string> lines = util::string::Split(source, "\n");

	uint32_t lineIndex = 1;
	size_t offset = 0;

	for (std::string& line : lines)
	{
		// This check, followed by the include statement check below isn't even remotely correct,
		// but we only have to care about shaders that we control here.
		if ((offset = line.find("//")) != std::string::npos)
			line = line.substr(0, offset);

		// The include path should be a relative path to the root of the shader directory.
		if ((offset = line.find("#include \"")) != std::string::npos)
		{
			std::string includePath = line.substr(offset + 10);
			if (!includePath.empty() && includePath.back() == '"')
				includePath.pop_back();

			// TODO: Remove this when we have project
			includePath = "BuiltInResources/Shaders/" + includePath;

			std::string includedSource;
			if (!FileSystem::ReadFileText(includePath, includedSource))
			{
				QK_CORE_LOGE_TAG("Renderer", "GLSLCompiler: Failed to include GLSL file: {}", includePath);
				return false;
			}

			std::string parsedIncludeSource;
			if (!ParseSource(includedSource, includePath, parsedIncludeSource))
				return false;

			// Add the include source to the result
			outParsedResult += util::string::Join("#line ", 1, " \"", includePath, "\"\n");
			std::vector<std::string> parsedIncludeSourceLines = util::string::Split(parsedIncludeSource, "\n");
			for (auto& incSourceline : parsedIncludeSourceLines)
				outParsedResult += incSourceline + "\n";
			outParsedResult += util::string::Join("#line ", lineIndex + 1, " \"", sourcePath, "\"\n");

			m_includeDependencies.insert(includePath);
		}
		else
		{
			outParsedResult += line + "\n";

			auto first_non_space = line.find_first_not_of(' ');

			if (first_non_space != std::string::npos && line[first_non_space] == '#')
			{
				auto keywords = util::string::Split(line.substr(first_non_space + 1), " ");
				if (keywords.size() == 1)
				{
					auto& word = keywords.front();
					if (word == "endif")
						outParsedResult += util::string::Join("#line ", lineIndex + 1, " \"", sourcePath, "\"\n");
				}
			}
		}

		lineIndex++;
	}

	return true;
}

};