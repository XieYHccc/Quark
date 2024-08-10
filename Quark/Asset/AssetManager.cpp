#include "Quark/Asset/AssetManager.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

#include "Quark/Core/FileSystem.h"

namespace quark {

static std::filesystem::path s_AssetDirectory = "Assets";
static std::filesystem::path s_AssetRegistryPath = "Assets/AssetRegistry.qkr";

static AssetType  GetAssetTypeFromString(std::string_view assetType)
{
	if (assetType == "MaxEnum")                return AssetType::MAX_ENUM;
	if (assetType == "Scene")				   return AssetType::SCENE;
	if (assetType == "Mesh")				   return AssetType::MESH;
	if (assetType == "Material")			   return AssetType::MATERIAL;
	if (assetType == "Texture")				   return AssetType::TEXTURE;
	if (assetType == "Shader")				   return AssetType::SHADER;
	if (assetType == "Script")				   return AssetType::SCRIPT;
	if (assetType == "Audio")				   return AssetType::AUDIO;

}
AssetManager::AssetManager()
{

}
Ref<Asset> AssetManager::GetAsset(AssetID id)
{
	return Ref<Asset>();
}

Ref<Asset> AssetManager::LoadAsset(AssetID id)
{
	return Ref<Asset>();
}

void AssetManager::RemoveAsset(AssetID id)
{

}

AssetType AssetManager::GetAssetTypeFromPath(const std::filesystem::path& filepath)
{
	return AssetType();
}

AssetType AssetManager::GetAssetTypeFromExtension(const std::string& extension)
{
	return AssetType();
}

void AssetManager::RegisterAsset(AssetMetadata metaData)
{
	CORE_ASSERT(metaData.IsValid())
	m_AssetMetadata[metaData.m_Id] = metaData;
}

void AssetManager::UnRegisterAsset(AssetID id)
{
	CORE_LOGI("Unregister Asset: {} from Registry")
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
		metadata.m_Id = a["Id"].as<uint64_t>();
		metadata.m_FilePath = filepath;
		metadata.m_Type = GetAssetTypeFromString(a["Type"].as<std::string>());

		if (metadata.m_Type == AssetType::MAX_ENUM)
			continue;


	}


}

void AssetManager::SaveAssetRegistry()
{
}

void AssetManager::ReloadAssets()
{
}

}