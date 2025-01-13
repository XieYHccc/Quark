#pragma once
#include <string>
#include "Quark/Core/UUID.h"

namespace quark {

using AssetID = UUID;

enum class AssetType : uint16_t {
	None,
	SCENE,
	MESH,
	MATERIAL,
	SKELETON,
	ANIMATION,
	SHADER,
	SCRIPT,
	AUDIO,
	FONT,
	IMAGE
};
 
// An asset is globally unique and can be identified by its AssetID
class Asset {
public:
	Asset() = default;
	virtual ~Asset() = default;

	virtual AssetType GetAssetType() = 0;
	static AssetType GetStaticAssetType() { return AssetType::None; }

	AssetID GetAssetID() const { return m_AssetID; }
	AssetID SetAssetID(AssetID id) { return m_AssetID = id; }
	
	void SetName(const std::string& name) { m_Name = name; }
	std::string GetName() const { return m_Name; }

	bool operator==(const Asset& other) const
	{
		return m_AssetID == other.m_AssetID;
	}
	bool operator!=(const Asset& other) const
	{
		return !(*this == other);
	}

private:
	AssetID m_AssetID;
	std::string m_Name;

	friend class AssetManager; // If the Asset is reloaded from AssetManager, AssetManager should be able to set the ID of the Asset
};

#define QUARK_ASSET_TYPE_DECL(type) \
	virtual AssetType GetAssetType() override { return GetStaticAssetType(); } \
	static AssetType GetStaticAssetType() { return AssetType::type; }
}
