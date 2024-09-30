#include "Quark/qkpch.h"
#include "Quark/Asset/AssetManager.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

#include "Quark/Core/FileSystem.h"
#include "Quark/Core/Application.h"
#include "Quark/Core/Util/StringUtils.h"
#include "Quark/Renderer/Renderer.h"
#include "Quark/Asset/AssetExtensions.h"
#include "Quark/Asset/MeshImporter.h"
#include "Quark/Asset/TextureImporter.h"
#include "Quark/Asset/MaterialSerializer.h"

namespace quark {
static std::filesystem::path s_AssetDirectory = "Assets";
static std::filesystem::path s_AssetRegistryPath = "Assets/AssetRegistry.qkr";

static AssetMetadata s_NullMetadata;

static AssetType  GetAssetTypeFromString(std::string_view assetType)
{
	if (assetType == "None")                return AssetType::None;
	if (assetType == "Scene")				   return AssetType::SCENE;
	if (assetType == "Mesh")				   return AssetType::MESH;
	if (assetType == "Material")			   return AssetType::MATERIAL;
	if (assetType == "Texture")				   return AssetType::TEXTURE;
	if (assetType == "Shader")				   return AssetType::SHADER;
	if (assetType == "Script")				   return AssetType::SCRIPT;
	if (assetType == "Audio")				   return AssetType::AUDIO;

	return AssetType::None;
}

static std::string AssetTypeToString(AssetType type)
{
	switch (type) {
	case AssetType::SCENE: 					   return "Scene";
	case AssetType::MESH: 					   return "Mesh";
	case AssetType::MATERIAL: 				   return "Material";
	case AssetType::TEXTURE: 				   return "Texture";
	case AssetType::SHADER: 				   return "Shader";
	case AssetType::SCRIPT: 				   return "Script";
	case AssetType::AUDIO: 					   return "Audio";
	default: 								   return "None";
	}
}

AssetManager::AssetManager()
{
	CreateDefaultAssets();
	LoadAssetRegistry();

	QK_CORE_LOGI_TAG("AssetManager", "AssetManager Initialized");
}

Ref<Asset> AssetManager::GetAsset(AssetID id)
{
	Ref<Asset> asset = nullptr;

	AssetMetadata metadata = GetAssetMetadata(id);

	if (metadata.IsValid()) 
	{
		if (metadata.isDataLoaded)
		{
			asset = m_LoadedAssets[id];
			return asset;
		}
		else 
		{
			// Load asset data
			switch (metadata.type) {
			case AssetType::MESH:
			{
				// TODO: Use MeshSerializer instead of MeshImporter
				MeshImporter meshImporter;
				asset = meshImporter.ImportGLTF(metadata.filePath.string());
				break;
			}
			case AssetType::TEXTURE:
			{
				// TODO: Use TextureSerializer instead of TextureImporter
				TextureImporter textureImporter;
				asset = textureImporter.ImportStb(metadata.filePath.string());
				break;
			}
			case AssetType::MATERIAL:
			{
				MaterialSerializer matSerializer;
				Ref<Material> newMat = CreateRef<Material>();
				if (matSerializer.TryLoadData(metadata.filePath.string(), newMat))
					asset = newMat;
				break;
			}
			default:
				QK_CORE_VERIFY(0)
			}

			if (asset)
			{
				asset->SetAssetID(id);
				m_LoadedAssets[id] = asset;
				metadata.isDataLoaded = true;
				SetMetadata(id, metadata);
			}
		}
	}

	return asset;
}

bool AssetManager::IsAssetLoaded(AssetID id)
{
	return m_LoadedAssets.contains(id);
}

bool AssetManager::IsAssetIdValid(AssetID id)
{
	return GetAssetMetadata(id).IsValid();
}

void AssetManager::RemoveAsset(AssetID id)
{
	if (m_LoadedAssets.contains(id))
		m_LoadedAssets.erase(id);

	if (m_AssetMetadata.contains(id))
		m_AssetMetadata.erase(id);
}

AssetType AssetManager::GetAssetTypeFromPath(const std::filesystem::path& filepath)
{
	return GetAssetTypeFromExtension(filepath.extension().string());
	
}

AssetType AssetManager::GetAssetTypeFromExtension(const std::string& extension)
{
	std::string ext = util::string::ToLowerCopy(extension);
	if (s_AssetExtensionMap.find(ext) == s_AssetExtensionMap.end())
	{
		QK_CORE_LOGW_TAG("AssetManger", "No asset type found for extension{0}", ext);
		return AssetType::None;
	}

	return s_AssetExtensionMap.at(ext);
}

AssetType AssetManager::GetAssetTypeFromID(AssetID id)
{
	if (IsAssetIdValid(id))
		return GetAssetMetadata(id).type;
	else
		return AssetType::None;
}

AssetID AssetManager::GetAssetIDFromFilePath(const std::filesystem::path& filepath)
{
	return GetAssetMetadata(filepath).id;
}

std::unordered_set<AssetID> AssetManager::GetAllAssetsWithType(AssetType type)
{
	std::unordered_set<AssetID> result;

	for (auto& [id, metadata] : m_AssetMetadata)
	{
		if (metadata.type == type)
			result.insert(id);
	}

	return result;
}

void AssetManager::SetMetadata(AssetID id, AssetMetadata metaData)
{
	QK_CORE_VERIFY(metaData.IsValid())
	m_AssetMetadata[metaData.id] = metaData;
}

AssetID AssetManager::ImportAsset(const std::filesystem::path &filepath)
{

	if (auto metadata = GetAssetMetadata(filepath); metadata.IsValid())
	{
		QK_CORE_LOGW_TAG("AssetManager" ,"Asset already Imported with id{0}", uint64_t(metadata.id));
		return metadata.id;
	}

	AssetType type = GetAssetTypeFromPath(filepath);
	if (type == AssetType::None)
	{
		QK_CORE_LOGW_TAG("AssetManager", "Asset file{0} is not supported.", filepath.string());
		return 0;
	}

	AssetMetadata newMetadata;
	newMetadata.id = AssetID();
	newMetadata.filePath = filepath;
	newMetadata.type = type;
	SetMetadata(newMetadata.id, newMetadata);

	return newMetadata.id;
}

AssetMetadata AssetManager::GetAssetMetadata(AssetID id)
{
	if (m_AssetMetadata.contains(id))
		return m_AssetMetadata[id];

	return s_NullMetadata;
}

AssetMetadata AssetManager::GetAssetMetadata(const std::filesystem::path& filepath)
{
	for (auto& [id, metadata] : m_AssetMetadata)
	{
		if (metadata.filePath == filepath)
			return metadata;
	}

	return s_NullMetadata;
}

void AssetManager::LoadAssetRegistry()
{
	if (!FileSystem::Exists(s_AssetRegistryPath))
		return;

	// Parse registry file into yaml
	std::ifstream stream(s_AssetRegistryPath);
	QK_CORE_VERIFY(stream)
	std::stringstream strStream;
	strStream << stream.rdbuf();

	YAML::Node data = YAML::Load(strStream.str());

	auto assetMetadata = data["Assets"];
	if (!assetMetadata)
	{
		QK_CORE_LOGW_TAG("AssetManager", "No asset meta data in registry file");
		return;
	}

	// Parse
	for (auto a : assetMetadata)
	{
		std::string filepath = a["FilePath"].as<std::string>();

		AssetMetadata metadata;
		metadata.id = a["Id"].as<uint64_t>();
		metadata.filePath = filepath;
		metadata.type = GetAssetTypeFromString(a["Type"].as<std::string>());

		if (metadata.type == AssetType::None)
		{
			QK_CORE_LOGW_TAG("AssetManager", "Asset type is None for asset{0}, this should't happen", filepath);
			continue;
		}

		if (metadata.id == 0) 
		{
			QK_CORE_LOGW_TAG("AssetManager", "Asset ID is 0 for asset{0}, this should't happen", filepath);
			continue;
		}

		SetMetadata(metadata.id, metadata);

	}

	QK_CORE_LOGI_TAG("AssetManager", "Loaded {0} asset entries", m_AssetMetadata.size());
}
void AssetManager::SaveAssetRegistry()
{
	QK_CORE_LOGI_TAG("AssetManager", "Saving asset registry with{0} assets", m_AssetMetadata.size());

	YAML::Emitter out;
	out << YAML::BeginMap;
	out << YAML::Key << "Assets" << YAML::BeginSeq;

	for (auto& [id, entry] : m_AssetMetadata)
	{
		out << YAML::BeginMap;
		out << YAML::Key << "Id" << YAML::Value << entry.id;
		out << YAML::Key << "FilePath" << YAML::Value << entry.filePath.string();
		out << YAML::Key << "Type" << YAML::Value << AssetTypeToString(entry.type);
		out << YAML::EndMap;
	}
	out << YAML::EndSeq;
	out << YAML::EndMap;

	std::ofstream fout(s_AssetRegistryPath);
	fout << out.c_str();
}

void AssetManager::ReloadAssets()
{

}

void AssetManager::CreateDefaultAssets()
{
	// Create defalult texture
	m_DefaultColorTexture = CreateRef<Texture>();
	m_DefaultColorTexture->image = Renderer::Get().image_white;
	m_DefaultColorTexture->sampler = Renderer::Get().sampler_linear;
	m_DefaultColorTexture->SetName("Default color texture");
	
	m_DefaultMetalTexture = CreateRef<Texture>();
	m_DefaultMetalTexture->image = Renderer::Get().image_white;
	m_DefaultMetalTexture->sampler = Renderer::Get().sampler_linear;
	m_DefaultMetalTexture->SetName("Default metalic roughness texture");

	m_DefaultMaterial = CreateRef<Material>();
	m_DefaultMaterial->alphaMode = AlphaMode::OPAQUE;
	m_DefaultMaterial->baseColorTexture = m_DefaultColorTexture;
	m_DefaultMaterial->metallicRoughnessTexture = m_DefaultMetalTexture;
	m_DefaultMaterial->uniformBufferData.baseColorFactor = glm::vec4(1.0f);
	m_DefaultMaterial->uniformBufferData.metalicFactor = 1.0f;
	m_DefaultMaterial->uniformBufferData.roughNessFactor = 1.0f;
	// TODO: Remove hardcoded shader
	m_DefaultMaterial->shaderProgram = Renderer::Get().GetShaderLibrary().defaultStaticMeshProgram;
	m_DefaultMaterial->SetName("Default material");
	
	// All default assets' id is 1
	m_DefaultColorTexture->SetAssetID(1);
	m_DefaultMetalTexture->SetAssetID(1);
	m_DefaultMaterial->SetAssetID(1);

}

}