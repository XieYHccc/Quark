#pragma once
#include <string>
#include "Quark/Core/Util/EnumCast.h"
#include "Quark/Core/Util/Singleton.h"
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
	Ref<graphic::Shader> gpuShaderHandle;
};

class ShaderTemplate 
{
public:
	ShaderTemplate(const std::string& path, graphic::ShaderStage stage);
	
	ShaderTemplateVariant* GetOrCreateVariant(const VariantSignatureKey& key);

	std::string GetPath() const { return m_Path; }

private:
	std::string m_Path;
	graphic::ShaderStage m_Stage;
	GLSLCompiler m_Compiler;

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
		const graphic::RenderPassInfo& compatablerp,
		const std::vector<graphic::VertexAttribInfo>& attribs,
		const std::vector<graphic::VertexBindInfo>& vertBindInfo);

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

	void SetStage(graphic::ShaderStage stage, ShaderTemplate* tmp);
	ShaderTemplate* GetStage(graphic::ShaderStage stage) { return m_Stages[util::ecast(stage)]; }

	std::string GetVertShaderPath() const { return m_Stages[util::ecast(graphic::ShaderStage::STAGE_VERTEX)]->GetPath(); }
	std::string GetFragShaderPath() const { return m_Stages[util::ecast(graphic::ShaderStage::STAGE_FRAGEMNT)]->GetPath(); }
	std::string GetComputeShaderPath() const { return m_Stages[util::ecast(graphic::ShaderStage::STAGE_COMPUTE)]->GetPath(); }

private:
	ShaderTemplate* m_Stages[util::ecast(graphic::ShaderStage::MAX_ENUM)] = {};

	std::unordered_map<uint64_t, Scope<ShaderProgramVariant>> m_Variants;
};

class ShaderManager : public util::MakeSingleton<ShaderManager>
{
public:
	ShaderManager();

	ShaderProgram* GetOrCreateGraphicsProgram(const std::string& vert_path, const std::string& frag_path);
	ShaderProgram* GetOrCreateComputeProgram(const std::string& comp_path);

private:
	ShaderTemplate* GetOrCreateShaderTemplate(const std::string& path, graphic::ShaderStage stage);
	
	std::unordered_map<uint64_t, Scope<ShaderTemplate>> m_ShaderTemplates;
	std::unordered_map<uint64_t, Scope<ShaderProgram>> m_ShaderPrograms;
};

}