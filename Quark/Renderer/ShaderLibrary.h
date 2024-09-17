#pragma once
#include <string>
#include "Quark/Core/Util/EnumCast.h"
#include "Quark/Graphic/Device.h"
#include "Quark/Renderer/GLSLCompiler.h"

namespace quark {

struct VariantSignatureKey
{
	uint32_t meshAttributeMask = 0;

	uint64_t GetHash() const;
};

// A ShaderTemplateVariant instance contains a gpu shader resource
struct ShaderTemplateVariant
{
	VariantSignatureKey signatureKey;
	Ref<graphic::Shader> gpuShaderHandle;
	
	std::vector<uint32_t> spirv;	// maybe used for serialization
	util::Hash spirvHash;
};

class ShaderTemplate 
{
public:
	// if is a spv file, this becomes a static shader template(Runtime case)
	// we want all glsl files to be compiled to spv at the first time you run the application.
	ShaderTemplate(const std::string& path, graphic::ShaderStage stage);

	// static shader template won't be able to (compile)create any variant
	ShaderTemplateVariant* GetOrCreateVariant(const VariantSignatureKey& key);
	ShaderTemplateVariant* GetPrecompiledVariant();

	std::string GetPath() const { return m_Path; }

	bool IsStatic() const { return m_Compiler == nullptr; } // we don't compile anything in static template

private:
	std::string m_Path;
	graphic::ShaderStage m_Stage;

	Scope<GLSLCompiler> m_Compiler;
	std::unordered_map<uint64_t, Scope<ShaderTemplateVariant>> m_Variants;
};

// This class can be represented as a combination of Ref<graphic::Shader> and 
// manages(caches) the Ref<PipeLine>s build from these shaders
class ShaderProgramVariant 
{
public:
	ShaderProgramVariant(ShaderTemplateVariant* vert, ShaderTemplateVariant* frag);
	ShaderProgramVariant(ShaderTemplateVariant* compute);

	Ref<graphic::PipeLine> GetOrCreatePipeLine(const graphic::PipelineDepthStencilState& ds, 
		const graphic::PipelineColorBlendState& cb,
		const graphic::RasterizationState& rs,
		const graphic::RenderPassInfo2& compatablerp,
		const graphic::VertexInputLayout& input);

private:
	ShaderTemplateVariant* m_Stages[util::ecast(graphic::ShaderStage::MAX_ENUM)] = {};

	std::unordered_map<uint64_t, Ref<graphic::PipeLine>> m_PipeLines;
};

// You should create a ShaderProgram instance through ShaderManager's API
class ShaderProgram 
{
public:
	ShaderProgram(ShaderTemplate* compute);
	ShaderProgram(ShaderTemplate* vert, ShaderTemplate* frag);

	ShaderProgramVariant* GetOrCreateVariant(const VariantSignatureKey& key);
	ShaderProgramVariant* GetPrecompiledVariant();

	std::string GetSourcePath(graphic::ShaderStage stage) const { return m_Stages[util::ecast(stage)]->GetPath(); }

	bool IsStatic() const;

private:
	ShaderTemplate* m_Stages[util::ecast(graphic::ShaderStage::MAX_ENUM)] = {};
	std::unordered_map<uint64_t, Scope<ShaderProgramVariant>> m_Variants;
};

class ShaderLibrary
{
public:
	ShaderProgram* defaultStaticMeshProgram = nullptr;
	ShaderProgram* staticProgram_skybox = nullptr;

	ShaderLibrary();

	ShaderProgram* GetOrCreateGraphicsProgram(const std::string& vert_path, const std::string& frag_path);
	ShaderProgram* GetOrCreateComputeProgram(const std::string& comp_path);

private:
	ShaderTemplate* GetOrCreateShaderTemplate(const std::string& path, graphic::ShaderStage stage);

	std::unordered_map<uint64_t, Scope<ShaderTemplate>> m_ShaderTemplates;
	std::unordered_map<uint64_t, Scope<ShaderProgram>> m_ShaderPrograms;
	
};

}