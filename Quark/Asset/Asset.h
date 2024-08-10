#pragma once
#include "Quark/Core/UUID.h"

namespace quark {

using AssetID = UUID;

enum class AssetType : uint16_t {
	SCENE,
	MESH,
	MATERIAL,
	TEXTURE,
	SHADER,
	SCRIPT,
	AUDIO,
	FONT,
	MAX_ENUM
};
 
class Asset {
public:
	Asset() = default;
	virtual ~Asset() = default;
	virtual AssetType GetAssetType() { return AssetType::MAX_ENUM; }
	static AssetType GetStaticAssetType() { return AssetType::MAX_ENUM; }

	AssetID GetID() const { return m_AssetID; }

	virtual bool operator==(const Asset& other) const
	{
		return m_AssetID == other.m_AssetID;
	}
	virtual bool operator!=(const Asset& other) const
	{
		return !(*this == other);
	}

private:
	AssetID m_AssetID;
	friend class AssetManager; // If the Asset is reloaded from AssetManager, AssetManager should be able to set the ID of the Asset
};

#define QUARK_ASSET_TYPE_DECL(type) \
	virtual AssetType GetAssetType() override { return GetStaticAssetType(); } \
	static AssetType GetStaticAssetType() { return AssetType::type; }
}
