#include "Quark/qkpch.h"
#include "Quark/Renderer/ShaderLibrary.h"
#include "Quark/Core/Application.h"
#include "Quark/Core/FileSystem.h"
#include "Quark/Core/Util/Hash.h"
#include "Quark/Asset/Mesh.h"

namespace quark {

uint64_t ShaderVariantKey::GetHash() const
{
	util::Hasher hasher;
	hasher.u32(meshAttributeMask);

	return hasher.get();
}


ShaderProgram::ShaderProgram(ShaderTemplate* compute)
{
	m_stages[util::ecast(graphic::ShaderStage::STAGE_COMPUTE)] = compute;
}

ShaderProgram::ShaderProgram(ShaderTemplate* vert, ShaderTemplate* frag)
{
	m_stages[util::ecast(graphic::ShaderStage::STAGE_VERTEX)] = vert;
	m_stages[util::ecast(graphic::ShaderStage::STAGE_FRAGEMNT)] = frag;
}

ShaderProgramVariant* ShaderProgram::GetOrCreateVariant(const ShaderVariantKey& key)
{
	uint64_t hash = key.GetHash();

	auto it = m_variants.find(hash);
	if (it != m_variants.end())
	{
		return it->second.get();
	}
	else
	{
		ShaderTemplateVariant* vert = m_stages[util::ecast(graphic::ShaderStage::STAGE_VERTEX)]->GetOrCreateVariant(key);
		ShaderTemplateVariant* frag = m_stages[util::ecast(graphic::ShaderStage::STAGE_FRAGEMNT)]->GetOrCreateVariant(key);

		Scope<ShaderProgramVariant> newVariant = CreateScope<ShaderProgramVariant>(vert, frag);

		m_variants[hash] = std::move(newVariant);

		return m_variants[hash].get();
	}
	return nullptr;
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
		ShaderTemplateVariant* vert = m_stages[util::ecast(graphic::ShaderStage::STAGE_VERTEX)]->GetPrecompiledVariant();
		ShaderTemplateVariant* frag = m_stages[util::ecast(graphic::ShaderStage::STAGE_FRAGEMNT)]->GetPrecompiledVariant();
		Scope<ShaderProgramVariant> newProgram = CreateScope<ShaderProgramVariant>(vert, frag);
		
		m_variants[hash] = std::move(newProgram);
		return m_variants[hash].get();
	}
}

bool ShaderProgram::IsStatic() const
{
	if(m_stages[util::ecast(graphic::ShaderStage::STAGE_COMPUTE)])
		return m_stages[util::ecast(graphic::ShaderStage::STAGE_COMPUTE)]->IsStatic();
	else
	{
		return m_stages[util::ecast(graphic::ShaderStage::STAGE_VERTEX)]->IsStatic() &&
			m_stages[util::ecast(graphic::ShaderStage::STAGE_FRAGEMNT)]->IsStatic();
	}

}

ShaderLibrary::ShaderLibrary()
{

	program_staticMesh = GetOrCreateGraphicsProgram("BuiltInResources/Shaders/static_mesh.vert",
		"BuiltInResources/Shaders/static_mesh.frag");

	program_staticMeshEditor = GetOrCreateGraphicsProgram("BuiltInResources/Shaders/editor_scene.vert",
		"BuiltInResources/Shaders/editor_scene.frag");

	staticProgram_skybox = GetOrCreateGraphicsProgram("BuiltInResources/Shaders/Spirv/skybox.vert.spv",
		"BuiltInResources/Shaders/Spirv/skybox.frag.spv");

	staticProgram_infiniteGrid = GetOrCreateGraphicsProgram("../../BuiltInResources/Shaders/Spirv/infinite_grid.vert.spv",
		"../../BuiltInResources/Shaders/Spirv/infinite_grid.frag.spv");
	
	staticProgram_entityID = GetOrCreateGraphicsProgram("../../BuiltInResources/Shaders/Spirv/entityID.vert.spv",
		"../../BuiltInResources/Shaders/Spirv/entityID.frag.spv");

	QK_CORE_LOGI_TAG("Renderer", "ShaderLibrary Initialized");
}

ShaderProgram* ShaderLibrary::GetOrCreateGraphicsProgram(const std::string& vert_path, const std::string& frag_path)
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
		ShaderTemplate* vertTemp = GetOrCreateShaderTemplate(vert_path, graphic::ShaderStage::STAGE_VERTEX);
		ShaderTemplate* fragTemp = GetOrCreateShaderTemplate(frag_path, graphic::ShaderStage::STAGE_FRAGEMNT);

		Scope<ShaderProgram> newProgram = CreateScope<ShaderProgram>(vertTemp, fragTemp);

		m_shaderPrograms[programHash] = std::move(newProgram);

		return m_shaderPrograms[programHash].get();
	}

}

ShaderProgram* ShaderLibrary::GetOrCreateComputeProgram(const std::string& comp_path)
{
	return nullptr;
}

ShaderTemplate* ShaderLibrary::GetOrCreateShaderTemplate(const std::string& path, graphic::ShaderStage stage)
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


ShaderTemplate::ShaderTemplate(const std::string& path, graphic::ShaderStage stage)
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

ShaderTemplateVariant* ShaderTemplate::GetOrCreateVariant(const ShaderVariantKey &key)
{
	if (IsStatic())
	{
		QK_CORE_LOGW_TAG("Renderer", "You can't create a variant from a static shader template");
		return nullptr;
	}

	uint64_t hash = key.GetHash();

	auto it = m_Variants.find(hash);
	if (it != m_Variants.end())
	{
		return it->second.get();
	}
	else
	{
		// Compile glsl shader with new key
		GLSLCompiler::CompileOptions ops;
		if (key.meshAttributeMask & MESH_ATTRIBUTE_POSITION_BIT)
			ops.AddDefine("HAVE_POSITION");
		if (key.meshAttributeMask & MESH_ATTRIBUTE_NORMAL_BIT)
			ops.AddDefine("HAVE_NORMAL");
		if (key.meshAttributeMask & MESH_ATTRIBUTE_UV_BIT)
			ops.AddDefine("HAVE_UV");
		if (key.meshAttributeMask & MESH_ATTRIBUTE_VERTEX_COLOR_BIT)
			ops.AddDefine("HAVE_VERTEX_COLOR");

		std::string messages;
		std::vector<uint32_t> spirv;
		if (!m_compiler->Compile(messages, spirv, ops))
		{
			QK_CORE_LOGE_TAG("Renderer", "ShaderTemplate: Failed to compile shader : {} : {}", m_path, messages);
			return nullptr;
		}

		Ref<graphic::Shader> newShader = Application::Get().GetGraphicDevice()->CreateShaderFromBytes(m_stage, spirv.data(), spirv.size() * sizeof(uint32_t));

		Scope<ShaderTemplateVariant> newVariant = CreateScope<ShaderTemplateVariant>();
		newVariant->gpuShaderHandle = newShader;
		newVariant->signatureKey = key;
		newVariant->spirv = spirv;

		m_Variants[hash] = std::move(newVariant);
		return m_Variants[hash].get();
	}
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

		Ref<graphic::Shader> newShader = Application::Get().GetGraphicDevice()->CreateShaderFromBytes(m_stage, spirv.data(), spirv.size());
		Scope<ShaderTemplateVariant> newVariant = CreateScope<ShaderTemplateVariant>();
		newVariant->gpuShaderHandle = newShader;
		newVariant->signatureKey = ShaderVariantKey();
		newVariant->spirv = std::vector<uint32_t>(spirv.begin(), spirv.end());
		m_Variants[hash] = std::move(newVariant);

		return m_Variants[hash].get();
	}
}
ShaderProgramVariant::ShaderProgramVariant(ShaderTemplateVariant* vert, ShaderTemplateVariant* frag)
{
	m_stages[util::ecast(graphic::ShaderStage::STAGE_VERTEX)] = vert;
	m_stages[util::ecast(graphic::ShaderStage::STAGE_FRAGEMNT)] = frag;
}

uint64_t ShaderProgramVariant::GetHash() const
{
	util::Hasher h;
	h.u64(m_stages[util::ecast(graphic::ShaderStage::STAGE_VERTEX)]->spirvHash);
	h.u64(m_stages[util::ecast(graphic::ShaderStage::STAGE_FRAGEMNT)]->spirvHash);

	return h.get();
}

}