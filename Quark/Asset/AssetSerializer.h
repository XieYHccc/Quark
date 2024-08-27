#pragma once
#include "Quark/Asset/Asset.h"
#include "Quark/Asset/AssetMetadata.h"
#include "Quark/Asset/Material.h"

namespace quark {

class AssetSerializer 
{
public:
	virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const = 0;
	virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const = 0;

	//TODO: Serialize and deserialize from AssetPack
	
};

class MaterialSerializer : public AssetSerializer
{
public:
	virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override;
	virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override;

private:
	std::string SerializeToYAML(Ref<Material> materialAsset) const;
	bool DeserializeFromYAML(const std::string& yamlString, Ref<Material>& targetMaterialAsset, AssetID id) const;
};

};