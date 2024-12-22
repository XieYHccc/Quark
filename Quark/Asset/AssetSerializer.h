#pragma once
#include "Quark/Asset/Asset.h"
#include "Quark/Asset/AssetMetadata.h"
namespace quark {

class AssetSerializer 
{
public:
	virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const = 0;
	virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const = 0;

	//TODO: Serialize and deserialize from AssetPack
	
};

};