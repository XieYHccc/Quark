#pragma once
#include "Quark/Core/Base.h"
#include "Quark/Core/Util/Singleton.h"
#include "Quark/Asset/Asset.h"
#include "Quark/Asset/AssetMetadata.h"
#include "Quark/Asset/Texture.h"
#include "Quark/Asset/Material.h"
#include "Quark/Asset/Mesh.h"

#include <unordered_set>

namespace quark {

class AssetManager : public util::MakeSingleton<AssetManager> {

public:
	// All default assets' id is 1
	Ref<Texture> defaultColorTexture;
	Ref<Texture> defaultMetalTexture;
	Ref<Material> defaultMaterial;
	Ref<Mesh> mesh_cube;

public:
	AssetManager();

	template<typename T>
	Ref<T> GetAsset(AssetID id);
	Ref<Asset> GetAsset(AssetID id);

	bool IsAssetIdValid(AssetID id);	// Is AssetID has a backup metadata? This has nothing to do with the actual asset data
	bool IsAssetLoaded(AssetID id);		// Is Asset has been loaded into memory?

	std::unordered_set<AssetID> GetAllAssetsWithType(AssetType type);

	AssetID ImportAsset(const std::filesystem::path& filepath);
	void RemoveAsset(AssetID id);

	AssetType GetAssetTypeFromPath(const std::filesystem::path& filepath);
	AssetType GetAssetTypeFromExtension(const std::string& extension);
	AssetType GetAssetTypeFromID(AssetID id);

	AssetID GetAssetIDFromFilePath(const std::filesystem::path& filepath);

	AssetMetadata GetAssetMetadata(AssetID id);
	AssetMetadata GetAssetMetadata(const std::filesystem::path& filepath);

	void LoadAssetRegistry();
	void SaveAssetRegistry();

private:
	void SetMetadata(AssetID id, AssetMetadata metaData);
	void ReloadAssets();
	void CreateDefaultAssets();

	std::unordered_map<AssetID, Ref<Asset>> m_LoadedAssets;
	std::unordered_map<AssetID, AssetMetadata> m_AssetMetadata;
};

template<typename T>
Ref<T> AssetManager::GetAsset(AssetID id)
{
	return std::static_pointer_cast<T>(GetAsset(id));
}

}