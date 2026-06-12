#include "Quark/qkpch.h"
#include "Quark/Render/RenderSystem.h"
#include "Quark/Core/FileSystem.h"
#include "Quark/Core/Util/Hash.h"
#include "Quark/Asset/MeshAsset.h"

namespace quark {

//uint64_t ShaderVariantKey::GetHash() const
//{
//	util::Hasher hasher;
//	hasher.u32(meshAttributeMask);
//
//	return hasher.get();
//}


ShaderProgram::ShaderProgram(ShaderTemplate* compute)
{
	m_stages[util::ecast(rhi::ShaderStage::STAGE_COMPUTE)] = compute;
}

ShaderProgram::ShaderProgram(ShaderTemplate* vert, ShaderTemplate* frag)
{
	m_stages[util::ecast(rhi::ShaderStage::STAGE_VERTEX)] = vert;
	m_stages[util::ecast(rhi::ShaderStage::STAGE_FRAGEMNT)] = frag;

	util::Hasher h;
	h.string(vert->GetPath());
	h.string(frag->GetPath());
	m_hash = h.get();
}

ShaderTemplateVariant* ShaderTemplate::RequestVariant(const std::vector<std::pair<std::string, int>>& defines)
{
	util::Hasher hasher;

	for (const auto& [s, v] : defines)
	{
		hasher.string(s);
		hasher.u32(v);
	}

	util::Hash hash = hasher.get();
	auto it = m_Variants.find(hash);
	if (it != m_Variants.end())
	{
		return it->second.get();
	}
	else
	{
		// Compile glsl shader with new key
		GLSLCompiler::CompileOptions ops;
		for (const auto& [s, v] : defines)
		{
			if (v == 1)
				ops.AddDefine(s);
			else if (v == 0)
				ops.AddUndefine(s);
		}

		std::string messages;
		std::vector<uint32_t> spirv;
		if (!m_compiler->Compile(messages, spirv, ops))
		{
			QK_CORE_LOGE_TAG("Renderer", "ShaderTemplate: Failed to compile shader : {} : {}", m_path, messages);
			return nullptr;
		}
		Ref<rhi::Shader> newShader = RenderSystem::Get().GetDevice()->CreateShaderFromBytes(m_stage, spirv.data(), spirv.size() * sizeof(uint32_t));

		Scope<ShaderTemplateVariant> newVariant = CreateScope<ShaderTemplateVariant>();
		newVariant->gpuShaderHandle = newShader;
		// newVariant->signatureKey = key;
		newVariant->spirv = spirv;

		m_Variants[hash] = std::move(newVariant);
		return m_Variants[hash].get();

	}
}

ShaderProgramVariant* ShaderProgram::RequestVariant(const std::vector<std::pair<std::string, int>>& defines)
{
	util::Hasher hasher;

	for (const auto& [s, v] : defines)
	{
		hasher.string(s);
		hasher.u32(v);
	}

	auto it = m_variants.find(hasher.get());
	if (it != m_variants.end())
	{
		return it->second.get();
	}
	else
	{
		ShaderTemplateVariant* vert = m_stages[util::ecast(rhi::ShaderStage::STAGE_VERTEX)]->RequestVariant(defines);
		ShaderTemplateVariant* frag = m_stages[util::ecast(rhi::ShaderStage::STAGE_FRAGEMNT)]->RequestVariant(defines);

		Scope<ShaderProgramVariant> newVariant = CreateScope<ShaderProgramVariant>(vert, frag);

		m_variants[hasher.get()] = std::move(newVariant);

		return m_variants[hasher.get()].get();

	}
}

ShaderProgramVariant* ShaderProgram::GetPrecompiledVariant()
{
	if (!IsStatic())
	{
		QK_CORE_LOGW_TAG("Renderer", "Only static shader programe has precompiled variant");
		return nullptr;
	}

	util::Hasher h;
	util::Hash hash = h.get();

	auto it = m_variants.find(hash);
	if (it != m_variants.end())
	{
		return it->second.get();
	}
	else
	{
		ShaderTemplateVariant* vert = m_stages[util::ecast(rhi::ShaderStage::STAGE_VERTEX)]->GetPrecompiledVariant();
		ShaderTemplateVariant* frag = m_stages[util::ecast(rhi::ShaderStage::STAGE_FRAGEMNT)]->GetPrecompiledVariant();
		Scope<ShaderProgramVariant> newProgram = CreateScope<ShaderProgramVariant>(vert, frag);
		
		m_variants[hash] = std::move(newProgram);
		return m_variants[hash].get();
	}
}

bool ShaderProgram::IsStatic() const
{
	if(m_stages[util::ecast(rhi::ShaderStage::STAGE_COMPUTE)])
		return m_stages[util::ecast(rhi::ShaderStage::STAGE_COMPUTE)]->IsStatic();
	else
	{
		return m_stages[util::ecast(rhi::ShaderStage::STAGE_VERTEX)]->IsStatic() &&
			m_stages[util::ecast(rhi::ShaderStage::STAGE_FRAGEMNT)]->IsStatic();
	}

}

ShaderLibrary::ShaderLibrary()
{
	program_staticMesh = RequestGraphicsProgram("builtin://Shaders/static_mesh.vert",
		"builtin://Shaders/static_mesh.frag");

	program_staticMeshEditor = RequestGraphicsProgram("builtin://Shaders/editor_scene.vert",
		"builtin://Shaders/editor_scene.frag");

	program_skybox = RequestGraphicsProgram("builtin://Shaders/skybox.vert",
		"builtin://Shaders/skybox.frag");

	staticProgram_infiniteGrid = RequestGraphicsProgram("builtin://Shaders/Spirv/infinite_grid.vert.spv",
		"builtin://Shaders/Spirv/infinite_grid.frag.spv");
	
	staticProgram_entityID = RequestGraphicsProgram("builtin://Shaders/Spirv/entityID.vert.spv",
		"builtin://Shaders/Spirv/entityID.frag.spv");

	QK_CORE_LOGI_TAG("Renderer", "ShaderLibrary Initialized");
}

ShaderProgram* ShaderLibrary::RequestGraphicsProgram(const std::string& vert_path, const std::string& frag_path)
{
	util::Hasher h;
	h.string(vert_path);
	h.string(frag_path);
	uint64_t programHash = h.get();

	auto it = m_shaderPrograms.find(programHash);
	if (it != m_shaderPrograms.end())
	{
		return it->second.get();
	}
	else // Create ShaderProgram
	{
		ShaderTemplate* vertTemp = RequestShaderTemplate(vert_path, rhi::ShaderStage::STAGE_VERTEX);
		ShaderTemplate* fragTemp = RequestShaderTemplate(frag_path, rhi::ShaderStage::STAGE_FRAGEMNT);

		Scope<ShaderProgram> newProgram = CreateScope<ShaderProgram>(vertTemp, fragTemp);

		m_shaderPrograms[programHash] = std::move(newProgram);

		return m_shaderPrograms[programHash].get();
	}

}

ShaderProgram* ShaderLibrary::RequestComputeProgram(const std::string& comp_path)
{
	return nullptr;
}

ShaderTemplate* ShaderLibrary::RequestShaderTemplate(const std::string& path, rhi::ShaderStage stage)
{
	util::Hasher h;
	h.string(path);

	auto it = m_shaderTemplates.find(h.get());
	if (it != m_shaderTemplates.end())
	{
		return it->second.get();
	}
	else
	{
		Scope<ShaderTemplate> newTemplate = CreateScope<ShaderTemplate>(path, stage);
		m_shaderTemplates[h.get()] = std::move(newTemplate);

		return m_shaderTemplates[h.get()].get();
	}
}


ShaderTemplate::ShaderTemplate(const std::string& path, rhi::ShaderStage stage)
	:m_path(path), m_stage(stage)
{
	if (FileSystem::GetExtension(path) == "spv")
	{
		// Static shader template
		return;
	}

	m_compiler = CreateScope<GLSLCompiler>();
	m_compiler->SetSourceFromFile(path, stage);
	m_compiler->SetTarget(GLSLCompiler::Target::VULKAN_VERSION_1_1);

}

ShaderTemplateVariant* ShaderTemplate::GetPrecompiledVariant()
{
	util::Hasher h;
	util::Hash hash = h.get();

	auto it = m_Variants.find(hash);
	if (it != m_Variants.end())
	{
		return it->second.get();
	}
	else
	{
		std::string messages;
		std::vector<uint8_t> spirv;
		if (!FileSystem::ReadFileBytes(m_path, spirv))
			return nullptr;

		Ref<rhi::Shader> newShader = RenderSystem::Get().GetDevice()->CreateShaderFromBytes(m_stage, spirv.data(), spirv.size());
		Scope<ShaderTemplateVariant> newVariant = CreateScope<ShaderTemplateVariant>();
		newVariant->gpuShaderHandle = newShader;
		// newVariant->signatureKey = ShaderVariantKey();
		newVariant->spirv = std::vector<uint32_t>(spirv.begin(), spirv.end());
		m_Variants[hash] = std::move(newVariant);

		return m_Variants[hash].get();
	}
}
ShaderProgramVariant::ShaderProgramVariant(ShaderTemplateVariant* vert, ShaderTemplateVariant* frag)
{
	m_stages[util::ecast(rhi::ShaderStage::STAGE_VERTEX)] = vert;
	m_stages[util::ecast(rhi::ShaderStage::STAGE_FRAGEMNT)] = frag;
}

uint64_t ShaderProgramVariant::GetHash() const
{
	util::Hasher h;
	h.u64(m_stages[util::ecast(rhi::ShaderStage::STAGE_VERTEX)]->spirvHash);
	h.u64(m_stages[util::ecast(rhi::ShaderStage::STAGE_FRAGEMNT)]->spirvHash);

	return h.get();
}

}