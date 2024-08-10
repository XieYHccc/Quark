#pragma once
#include "Quark/Core/Base.h"
#include "Quark/Core/Util/Singleton.h"
#include "Quark/Asset/Asset.h"
#include "Quark/Asset/AssetMetadata.h"

namespace quark {

class AssetManager : public util::MakeSingleton<AssetManager> {
public:
	AssetManager();
	Ref<Asset> GetAsset(AssetID id);
	Ref<Asset> LoadAsset(AssetID id);
	void RemoveAsset(AssetID id);
	AssetType GetAssetTypeFromPath(const std::filesystem::path& filepath);
	AssetType GetAssetTypeFromExtension(const std::string& extension);

	void RegisterAsset(AssetMetadata metaData);
	void UnRegisterAsset(AssetID id);

private:
	// Load metadata from disk
	void LoadAssetRegistry();
	void SaveAssetRegistry();
	void ReloadAssets();

	std::unordered_map<AssetID, Ref<Asset>> m_LoadedAssets;
	std::unordered_map<AssetID, AssetMetadata> m_AssetMetadata;
};

}