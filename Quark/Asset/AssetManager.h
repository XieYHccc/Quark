#pragma once
#include "Quark/Core/Base.h"
#include "Quark/Core/Util/Singleton.h"
#include "Quark/Asset/Asset.h"
#include "Quark/Asset/AssetMetadata.h"
#include "Quark/Asset/MeshAsset.h"
#include "Quark/Asset/MaterialAsset.h"
#include "Quark/Asset/ImageAsset.h"
#include "Quark/Project/Project.h"

#include <unordered_set>

namespace quark {

class AssetManager : public util::MakeSingleton<AssetManager> {
public:
	Ref<MeshAsset> mesh_cube;

public:
	AssetManager();
	void Init(); // init asset manager every time a new project is loaded

	template<typename T>
	Ref<T> GetAsset(AssetID id);
	Ref<Asset> GetAsset(AssetID id);

	void AddMemoryOnlyAsset(Ref<Asset> asset);
	bool IsAssetIdValid(AssetID id);	// Is AssetID has a backup metadata? This has nothing to do with the actual asset data
	bool IsAssetLoaded(AssetID id);		// Is Asset has been loaded into memory?

	std::unordered_set<AssetID> GetAllAssetsWithType(AssetType type);

	AssetType GetAssetTypeFromPath(const std::filesystem::path& filepath);
	AssetType GetAssetTypeFromExtension(const std::string& extension);
	AssetType GetAssetTypeFromID(AssetID id);

	AssetID GetAssetIDFromFilePath(const std::filesystem::path& filepath);
	AssetID ImportAsset(const std::filesystem::path& filepath);
	void RemoveAsset(AssetID id);
	
	AssetMetadata GetAssetMetadata(AssetID id);
	AssetMetadata GetAssetMetadata(const std::filesystem::path& filepath);

	void LoadAssetRegistry();
	void SaveAssetRegistry();

private:
	void SetMetadata(AssetID id, AssetMetadata metaData);
	void ReloadAssets();
	void CreateDefaultAssets();

	std::unordered_map<AssetID, Ref<Asset>> m_memoryOnlyAssets;
	std::unordered_map<AssetID, Ref<Asset>> m_loadedAssets;
	std::unordered_map<AssetID, AssetMetadata> m_assetMetadata;
};

template<typename T>
Ref<T> AssetManager::GetAsset(AssetID id)
{
	return std::static_pointer_cast<T>(GetAsset(id));
}

}