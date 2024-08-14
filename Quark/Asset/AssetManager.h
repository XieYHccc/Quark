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
	void RemoveAsset(AssetID id);

	bool IsAssetLoaded(AssetID id);

	AssetType GetAssetTypeFromPath(const std::filesystem::path& filepath);
	AssetType GetAssetTypeFromExtension(const std::string& extension);

	void RegisterAssetWithMetadata(AssetMetadata metaData);
	AssetMetadata GetAssetMetadata(AssetID id);
	
	// Load metadata from disk
	void LoadAssetRegistry();
	void SaveAssetRegistry();
	void ReloadAssets();

	std::unordered_map<AssetID, Ref<Asset>> m_LoadedAssets;
	std::unordered_map<AssetID, AssetMetadata> m_AssetMetadata;
};

}