#include "Quark/qkpch.h"
#include "Quark/Renderer/ShaderManager.h"
#include "Quark/Core/Util/Hash.h"

namespace quark {
uint64_t VariantSignatureKey::GetHash() const
{
	util::Hasher hasher;
	hasher.u32(meshAttributeMask);

	return hasher.get();
}


ShaderProgram::ShaderProgram(ShaderTemplate* compute)
{
	m_Stages[util::ecast(graphic::ShaderStage::STAGE_COMPUTE)] = compute;
}

ShaderProgram::ShaderProgram(ShaderTemplate* vert, ShaderTemplate* frag)
{
	m_Stages[util::ecast(graphic::ShaderStage::STAGE_VERTEX)] = vert;
	m_Stages[util::ecast(graphic::ShaderStage::STAGE_FRAGEMNT)] = frag;
}

ShaderProgramVariant* ShaderProgram::GetOrCreateVariant(const VariantSignatureKey& key)
{
	return nullptr;
}

ShaderManager::ShaderManager()
{
	CORE_LOGI("[ShaderManager]: Initialized");
}

ShaderProgram* ShaderManager::GetOrCreateGraphicsProgram(const std::string& vert_path, const std::string& frag_path)
{
	util::Hasher h;
	h.string(vert_path);
	h.string(frag_path);
	uint64_t programHash = h.get();


	auto it = m_ShaderPrograms.find(programHash);
	if (it != m_ShaderPrograms.end())
	{
		return it->second.get();
	}
	else // Create ShaderProgram
	{
		ShaderTemplate* vertTemp = GetOrCreateShaderTemplate(vert_path, graphic::ShaderStage::STAGE_VERTEX);
		ShaderTemplate* fragTemp = GetOrCreateShaderTemplate(frag_path, graphic::ShaderStage::STAGE_FRAGEMNT);

		Scope<ShaderProgram> newProgram = CreateScope<ShaderProgram>(vertTemp, fragTemp);

		m_ShaderPrograms[programHash] = std::move(newProgram);

		return m_ShaderPrograms[programHash].get();
	}

}

ShaderProgram* ShaderManager::GetOrCreateComputeProgram(const std::string& comp_path)
{
	return nullptr;
}

ShaderTemplate* ShaderManager::GetOrCreateShaderTemplate(const std::string& path, graphic::ShaderStage stage)
{
	util::Hasher h;
	h.string(path);

	auto it = m_ShaderTemplates.find(h.get());
	if (it != m_ShaderTemplates.end())
	{
		return it->second.get();
	}
	else
	{
		Scope<ShaderTemplate> newTemplate = CreateScope<ShaderTemplate>(path, stage);
		m_ShaderTemplates[h.get()] = std::move(newTemplate);
	}
}

Ref<graphic::PipeLine> ShaderProgramVariant::GetOrCreatePipeLine(const graphic::PipelineDepthStencilState& ds,
																 const graphic::PipelineColorBlendState& cb, 
																 const graphic::RasterizationState& rs, 
																 const graphic::RenderPassInfo& compatablerp)
{
	return Ref<graphic::PipeLine>();
}

ShaderTemplate::ShaderTemplate(const std::string& path, graphic::ShaderStage stage)
	:m_Path(path), m_Stage(stage)
{
	m_Compiler.SetSourceFromFile(path, stage);
	m_Compiler.SetTarget(GLSLCompiler::Target::VULKAN_VERSION_1_3);
}

}