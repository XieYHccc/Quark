#include "Quark/Asset/AssetManager.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

#include "Quark/Core/FileSystem.h"
#include "Quark/Core/Application.h"
#include "Quark/Asset/MeshLoader.h"

namespace quark {
static std::filesystem::path s_AssetDirectory = "Assets";
static std::filesystem::path s_AssetRegistryPath = "Assets/AssetRegistry.qkr";

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
	LoadAssetRegistry();
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
		}
		else 
		{
			// Load asset data
			switch (metadata.type) {
			case AssetType::MESH:
				{
					MeshLoader meshLoader(Application::Instance().GetGraphicDevice());
					asset = meshLoader.LoadGLTF(metadata.filePath.string());
					asset->SetAssetID(id);
					break;
				}
			default:
				CORE_ASSERT(0)
			}
		}
	}

	return asset;
}

bool AssetManager::IsAssetLoaded(AssetID id)
{
	return m_LoadedAssets.contains(id);
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
	return AssetType();
}

AssetType AssetManager::GetAssetTypeFromExtension(const std::string& extension)
{
	return AssetType();
}

void AssetManager::RegisterAssetWithMetadata(AssetMetadata metaData)
{
	CORE_ASSERT(metaData.IsValid())
	m_AssetMetadata[metaData.id] = metaData;
}

AssetMetadata AssetManager::GetAssetMetadata(AssetID id)
{
	if (m_AssetMetadata.contains(id))
		return m_AssetMetadata[id];

	return AssetMetadata();
}

void AssetManager::LoadAssetRegistry()
{
	if (!FileSystem::Exists(s_AssetRegistryPath))
		return;

	// Parse registry file into yaml
	std::ifstream stream(s_AssetRegistryPath);
	CORE_ASSERT(stream)
	std::stringstream strStream;
	strStream << stream.rdbuf();

	YAML::Node data = YAML::Load(strStream.str());

	auto assetMetadata = data["Assets"];
	if (!assetMetadata)
	{
		CORE_LOGE("No asset meta data in registry file")
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
			CORE_LOGW("[AssetManager] Asset type is None for asset {0}, this should't happen", filepath);
			continue;
		}

		if (metadata.id == 0) 
		{
			CORE_LOGW("[AssetManager] Asset ID is 0 for asset {0}, this should't happen", filepath);
			continue;
		}

		RegisterAssetWithMetadata(metadata);

	}

}
void AssetManager::SaveAssetRegistry()
{
	CORE_LOGI("[AssetManager] Saving asset registry with {0} assets", m_AssetMetadata.size());

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

}