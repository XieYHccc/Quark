#pragma once
#include "Quark/RHI/Shader.h"

#include <unordered_set>

namespace quark {

/// helper class to generate SPIRV code from GLSL source
/// currently only support compiling for one shader stage and vulkan 1.3
class GLSLCompiler {
public:
	// adds support for C style preprocessor macros to glsl shaders
	// enabling you to define or undefine certain symbols
	class CompileOptions {
	public:
		CompileOptions() = default;

		void AddDefinitions(const std::vector<std::string>& definitions);
		void AddDefine(const std::string& def);
		void AddUndefine(const std::string& undef);

		const std::string& GetPreamble() const { return m_Preamble; }
		const std::vector<std::string>& GetProcesses() const { return m_Processes; }

	private:
		void FixLine(std::string& line);

		std::string m_Preamble;
		std::vector<std::string> m_Processes;

	};

	enum class Target
	{
		VULKAN_VERSION_1_3,
		VULKAN_VERSION_1_1,
	};

	GLSLCompiler() = default;

	void SetTarget(Target target);

	void SetSource(std::string source, std::string sourcePath, rhi::ShaderStage stage);
	void SetSourceFromFile(const std::string& filePath, rhi::ShaderStage stage);

	bool Compile(std::string& outMessages, std::vector<uint32_t>& outSpirv, const CompileOptions& ops = {});

	void Clear();

private:
	void PreProcess();

	// Recursively parse the source and its includes
	bool ParseSource(const std::string& source, const std::string sourcePath, std::string& outParsedResult);

	std::string m_sourcePath;
	std::string m_source;
	std::string m_preprocessedSource;

	std::unordered_set<std::string> m_includeDependencies;

	Target m_target = Target::VULKAN_VERSION_1_1;

	rhi::ShaderStage m_shaderStage = rhi::ShaderStage::MAX_ENUM;

	bool m_isPreprocessed = false;
};
}