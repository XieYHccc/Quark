#pragma once
#include "Quark/Core/Util/EnumCast.h"
#include "Quark/Core/Util/Hash.h"
#include "Quark/Render/GLSLCompiler.h"

#include <string>

namespace quark {

struct ShaderVariantKey
{
	uint32_t meshAttributeMask = 0;

	uint64_t GetHash() const;
};

// a shaderTemplateVariant instance contains a gpu shader resource
struct ShaderTemplateVariant
{
	ShaderVariantKey signatureKey;
	Ref<rhi::Shader> gpuShaderHandle;
	
	std::vector<uint32_t> spirv;	// maybe used for serialization
	util::Hash spirvHash; // <=> Ref<rhi::Shader>
};

// shader source class
class ShaderTemplate 
{
public:
	// if is a spv file, this becomes a static shader template(Runtime case)
	// we want all glsl files to be compiled to spv at the first time you run the application.
	ShaderTemplate(const std::string& path, rhi::ShaderStage stage);

	// static shader template won't be able to (compile)create any variant
	ShaderTemplateVariant* GetOrCreateVariant(const ShaderVariantKey& key);
	ShaderTemplateVariant* GetPrecompiledVariant();

	std::string GetPath() const { return m_path; }

	bool IsStatic() const { return m_compiler == nullptr; } // we don't compile anything in static template

private:
	std::string m_path;
	rhi::ShaderStage m_stage;

	Scope<GLSLCompiler> m_compiler;
	std::unordered_map<uint64_t, Scope<ShaderTemplateVariant>> m_Variants;
};

// This class can be represented as a combination of Ref<rhi::Shader>
class ShaderProgramVariant 
{
public:
	ShaderProgramVariant(ShaderTemplateVariant* vert, ShaderTemplateVariant* frag);
	ShaderProgramVariant(ShaderTemplateVariant* compute);

	const Ref<rhi::Shader> GetShader(rhi::ShaderStage stage) const { return m_stages[util::ecast(stage)]->gpuShaderHandle; }

	uint64_t GetHash() const;

private:
	ShaderTemplateVariant* m_stages[util::ecast(rhi::ShaderStage::MAX_ENUM)] = {};

};

// You should create a ShaderProgram instance through ShaderManager's API
class ShaderProgram 
{
public:
	ShaderProgram(ShaderTemplate* compute);
	ShaderProgram(ShaderTemplate* vert, ShaderTemplate* frag);

	ShaderProgramVariant* GetOrCreateVariant(const ShaderVariantKey& key);
	ShaderProgramVariant* GetPrecompiledVariant();

	std::string GetSourcePath(rhi::ShaderStage stage) const { return m_stages[util::ecast(stage)]->GetPath(); }

	bool IsStatic() const;

private:
	ShaderTemplate* m_stages[util::ecast(rhi::ShaderStage::MAX_ENUM)] = {};
	std::unordered_map<uint64_t, Scope<ShaderProgramVariant>> m_variants;
};

class ShaderLibrary
{
public:
	ShaderProgram* program_staticMesh;
	ShaderProgram* program_staticMeshEditor;
	ShaderProgram* staticProgram_skybox;
	ShaderProgram* staticProgram_infiniteGrid;
	ShaderProgram* staticProgram_entityID;

public:
	ShaderLibrary();

	ShaderProgram* GetOrCreateGraphicsProgram(const std::string& vert_path, const std::string& frag_path);
	ShaderProgram* GetOrCreateComputeProgram(const std::string& comp_path);
	ShaderTemplate* GetOrCreateShaderTemplate(const std::string& path, rhi::ShaderStage stage);

private:
	std::unordered_map<uint64_t, Scope<ShaderTemplate>> m_shaderTemplates;
	std::unordered_map<uint64_t, Scope<ShaderProgram>> m_shaderPrograms;
	
};

}